package statemachine

import (
	"bytes"
	"errors"
	"fmt"
	cache "proxy/Cache"
	"strings"
	"syscall"
	"time"

	"golang.org/x/sys/unix"
)

const (
	STAGE_READ_FROM_CLIENT        = 1
	STAGE_WRITE_REQUEST_TO_SITE   = 2
	STAGE_READ_RESPONSE_FROM_SITE = 3
	STAGE_WRITE_TO_CLIENT         = 4
)

type StateMachine struct {
	ClientFd int
	SiteFd   int
	Stage    int

	buffer       []byte
	request      []byte
	response     []byte
	fullResponse []byte

	expectedBodyLen int
	headerProcessed bool

	url  string
	host string
	ip   []byte
	port int

	cache *cache.Cache
}

func NewStateMachine(ClientFd int, cache *cache.Cache) *StateMachine {
	return &StateMachine{
		ClientFd: ClientFd,
		SiteFd:   -1,
		Stage:    STAGE_READ_FROM_CLIENT,

		cache: cache,
	}
}

func (sm *StateMachine) cleanup(allFds *[]int, states map[int]*StateMachine) {
	if sm.ClientFd != -1 {
		delete(states, sm.ClientFd)
		deleteFd(sm.ClientFd, allFds)
		unix.Close(sm.ClientFd)
		sm.ClientFd = -1
	}
	if sm.SiteFd != -1 {
		delete(states, sm.SiteFd)
		deleteFd(sm.SiteFd, allFds)
		unix.Close(sm.SiteFd)
		sm.SiteFd = -1
	}
}

func (sm *StateMachine) createPortToSite(url, host string, allFds *[]int, states map[int]*StateMachine) {
	SiteFd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		fmt.Printf("Create site socket error on fd %d: %v\n", sm.ClientFd, err)
		sm.cleanup(allFds, states)
		return
	}

	if err = unix.SetNonblock(SiteFd, true); err != nil {
		fmt.Printf("Set nonblock error on fd %d: %v\n", sm.ClientFd, err)
		sm.cleanup(allFds, states)
		return
	}

	ip, port := resolveHostPort(sm.host, 80)
	siteAddress := &unix.SockaddrInet4{Port: port, Addr: ip}

	err = unix.Connect(SiteFd, siteAddress) //non-blocking

	if err == unix.EINPROGRESS || err == nil {
		sm.SiteFd = SiteFd
		sm.Stage = STAGE_WRITE_REQUEST_TO_SITE
		states[sm.SiteFd] = sm
		*allFds = append(*allFds, sm.SiteFd)
	} else {
		fmt.Printf("Connect error on fd %d: %v\n", sm.ClientFd, err)
		sm.cleanup(allFds, states)
		return
	}
}

func (sm *StateMachine) HandleClientRead(allFds *[]int, states map[int]*StateMachine) {
	buf := make([]byte, 4096)
	n, err := syscall.Read(sm.ClientFd, buf) //non-blocking

	if err == nil {
		if n == 0 {
			fmt.Printf("Client %d disconnected\n", sm.ClientFd)
			sm.cleanup(allFds, states)
			return
		}

		sm.buffer = append(sm.buffer, buf[:n]...)
		if bytes.Contains(sm.buffer, []byte("\r\n\r\n")) {
			sm.request = sm.buffer

			url, host, ok := parseRequest(sm.request)
			if !ok {
				fmt.Printf("Parse error on fd %d: %v\n", sm.ClientFd, errors.New("invalid HTTP request"))
				sm.cleanup(allFds, states)
				return
			}

			sm.url = url
			sm.host = host

			if entry, exist := sm.cache.Get(sm.url); exist && time.Since(entry.Timestamp) < 1*time.Minute {
				fmt.Printf("Get response from cache with url: %s\n", sm.url)
				sm.response = entry.Data
				sm.Stage = STAGE_WRITE_TO_CLIENT
			} else {
				fmt.Printf("Connecting to site with url: %s\n", sm.url)
				sm.createPortToSite(url, host, allFds, states)
			}
		}
	} else {
		if (err != syscall.EAGAIN) && (err != syscall.EWOULDBLOCK) {
			fmt.Printf("Read error on fd %d: %v\n", sm.ClientFd, err)
			sm.cleanup(allFds, states)
			return
		}
	}
}

func (sm *StateMachine) ProcessSite(allFds *[]int, states map[int]*StateMachine) {
	switch sm.Stage {
	case STAGE_WRITE_REQUEST_TO_SITE:
		val, err := unix.GetsockoptInt(sm.SiteFd, unix.SOL_SOCKET, unix.SO_ERROR)
		if err != nil {
			fmt.Printf("Getsockopt error on fd %d: %v\n", sm.SiteFd, err)
			sm.cleanup(allFds, states)
			return
		}

		if val == 0 {
			n, err := unix.Write(sm.SiteFd, sm.request) //non-blocking
			if err != nil {
				if err == unix.EAGAIN || err == unix.EWOULDBLOCK {
					return
				}

				fmt.Printf("Write error on fd %d: %v\n", sm.SiteFd, err)
				sm.cleanup(allFds, states)
				return
			}

			if n > 0 {
				sm.request = sm.request[n:]
				if len(sm.request) == 0 {
					sm.Stage = STAGE_READ_RESPONSE_FROM_SITE
				}
			}
		}

	case STAGE_READ_RESPONSE_FROM_SITE:
		buf := make([]byte, 4096)
		n, err := unix.Read(sm.SiteFd, buf)

		if err != nil {
			if err == unix.EAGAIN || err == unix.EWOULDBLOCK {
				return
			}

			fmt.Printf("Read error on fd %d: %v\n", sm.SiteFd, err)
			sm.cleanup(allFds, states)
			return
		}

		if n == 0 {
			if sm.headerProcessed {
				fmt.Printf("Server closed connection (EOF). Finalizing response for %s\n", sm.url)
				sm.fullResponse = make([]byte, len(sm.response))
				copy(sm.fullResponse, sm.response)
				sm.Stage = STAGE_WRITE_TO_CLIENT

				if sm.SiteFd != -1 {
					delete(states, sm.SiteFd)
					deleteFd(sm.SiteFd, allFds)
					unix.Close(sm.SiteFd)
					sm.SiteFd = -1
				}
			} else {
				sm.cleanup(allFds, states)
			}

			return
		}

		if n > 0 {
			sm.response = append(sm.response, buf[:n]...)

			if !sm.headerProcessed {
				if idx := bytes.Index(sm.response, []byte("\r\n\r\n")); idx != -1 {
					sm.headerProcessed = true

					headers := string(sm.response[:idx])
					sm.expectedBodyLen = parseContentLength(headers)

					if sm.expectedBodyLen == 0 && !strings.Contains(headers, "chunked") {
						sm.fullResponse = make([]byte, len(sm.response))
						copy(sm.fullResponse, sm.response)
						sm.Stage = STAGE_WRITE_TO_CLIENT
						return
					}
				}
			}

			if sm.headerProcessed {
				headerEnd := bytes.Index(sm.response, []byte("\r\n\r\n")) + 4
				currentBodyLen := len(sm.response) - headerEnd

				if currentBodyLen >= sm.expectedBodyLen {
					sm.fullResponse = make([]byte, len(sm.response))
					copy(sm.fullResponse, sm.response)
					sm.Stage = STAGE_WRITE_TO_CLIENT
				}
			}
		}

	case STAGE_WRITE_TO_CLIENT:
		n, err := unix.Write(sm.ClientFd, sm.response)
		if err != nil {
			if err == unix.EAGAIN || err == unix.EWOULDBLOCK {
				return
			}

			fmt.Printf("Write error on fd %d: %v\n", sm.ClientFd, err)
			sm.cleanup(allFds, states)
			return
		}

		if n > 0 {
			sm.response = sm.response[n:]
			fmt.Printf("Sent %d bytes to client, %d remaining\n", n, len(sm.response))
			if len(sm.response) == 0 {
				if _, exist := sm.cache.Get(sm.url); !exist {
					sm.cache.Set(sm.url, cache.CacheEntry{Data: sm.fullResponse, Timestamp: time.Now()})
				}

				sm.cleanup(allFds, states)
			}
		}
	}
}
