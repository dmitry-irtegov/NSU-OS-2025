package main

import (
	"bytes"
	"errors"
	"fmt"
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
	clientFd int
	siteFd   int
	stage    int

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
}

func (sm *StateMachine) cleanup(allFds *[]int, states map[int]*StateMachine) {
	if sm.clientFd != -1 {
		delete(states, sm.clientFd)
		deleteFd(sm.clientFd, allFds)
		unix.Close(sm.clientFd)
		sm.clientFd = -1
	}
	if sm.siteFd != -1 {
		delete(states, sm.siteFd)
		deleteFd(sm.siteFd, allFds)
		unix.Close(sm.siteFd)
		sm.siteFd = -1
	}
}

func (sm *StateMachine) createPortToSite(url, host string, allFds *[]int, states map[int]*StateMachine) {
	siteFd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		fmt.Printf("Create site socket error on fd %d: %v\n", sm.clientFd, err)
		sm.cleanup(allFds, states)
		return
	}

	if err = unix.SetNonblock(siteFd, true); err != nil {
		fmt.Printf("Set nonblock error on fd %d: %v\n", sm.clientFd, err)
		sm.cleanup(allFds, states)
		return
	}

	ip, port := resolveHostPort(sm.host, 80)
	siteAddress := &unix.SockaddrInet4{Port: port, Addr: ip}

	err = unix.Connect(siteFd, siteAddress) //non-blocking

	if err == unix.EINPROGRESS || err == nil {
		sm.siteFd = siteFd
		sm.stage = STAGE_WRITE_REQUEST_TO_SITE
		states[sm.siteFd] = sm
		*allFds = append(*allFds, sm.siteFd)
	} else {
		fmt.Printf("Connect error on fd %d: %v\n", sm.clientFd, err)
		sm.cleanup(allFds, states)
		return
	}
}

func (sm *StateMachine) handleClientRead(allFds *[]int, states map[int]*StateMachine) {
	buf := make([]byte, 4096)
	n, err := syscall.Read(sm.clientFd, buf) //non-blocking

	if err == nil {
		if n == 0 {
			fmt.Printf("Client %d disconnected\n", sm.clientFd)
			sm.cleanup(allFds, states)
			return
		}

		sm.buffer = append(sm.buffer, buf[:n]...)
		if bytes.Contains(sm.buffer, []byte("\r\n\r\n")) {
			sm.request = sm.buffer

			url, host, ok := parseRequest(sm.request)
			if !ok {
				fmt.Printf("Parse error on fd %d: %v\n", sm.clientFd, errors.New("invalid HTTP request"))
				sm.cleanup(allFds, states)
				return
			}

			sm.url = url
			sm.host = host

			if entry, exist := cache[url]; exist && time.Since(entry.Timestamp) < 1*time.Minute {
				fmt.Printf("Get response from cache with url: %s\n", sm.url)
				sm.response = entry.Data
				sm.stage = STAGE_WRITE_TO_CLIENT
			} else {
				fmt.Printf("Connecting to site with url: %s\n", sm.url)
				sm.createPortToSite(url, host, allFds, states)
			}
		}
	} else {
		if (err != syscall.EAGAIN) && (err != syscall.EWOULDBLOCK) {
			fmt.Printf("Read error on fd %d: %v\n", sm.clientFd, err)
			sm.cleanup(allFds, states)
			return
		}
	}
}

func (sm *StateMachine) processSite(allFds *[]int, states map[int]*StateMachine) {
	switch sm.stage {
	case STAGE_WRITE_REQUEST_TO_SITE:
		val, err := unix.GetsockoptInt(sm.siteFd, unix.SOL_SOCKET, unix.SO_ERROR)
		if err != nil {
			fmt.Printf("Getsockopt error on fd %d: %v\n", sm.siteFd, err)
			sm.cleanup(allFds, states)
			return
		}

		if val == 0 {
			n, err := unix.Write(sm.siteFd, sm.request) //non-blocking
			if err != nil {
				if err == unix.EAGAIN || err == unix.EWOULDBLOCK {
					return
				}

				fmt.Printf("Write error on fd %d: %v\n", sm.siteFd, err)
				sm.cleanup(allFds, states)
				return
			}

			if n > 0 {
				sm.request = sm.request[n:]
				if len(sm.request) == 0 {
					sm.stage = STAGE_READ_RESPONSE_FROM_SITE
				}
			}
		}

	case STAGE_READ_RESPONSE_FROM_SITE:
		buf := make([]byte, 4096)
		n, err := unix.Read(sm.siteFd, buf)

		if err != nil {
			if err == unix.EAGAIN || err == unix.EWOULDBLOCK {
				return
			}

			fmt.Printf("Read error on fd %d: %v\n", sm.siteFd, err)
			sm.cleanup(allFds, states)
			return
		}

		if n == 0 {
			if sm.headerProcessed {
				fmt.Printf("Server closed connection (EOF). Finalizing response for %s\n", sm.url)
				sm.fullResponse = make([]byte, len(sm.response))
				copy(sm.fullResponse, sm.response)
				sm.stage = STAGE_WRITE_TO_CLIENT
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
						sm.stage = STAGE_WRITE_TO_CLIENT
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
					sm.stage = STAGE_WRITE_TO_CLIENT
				}
			}
		}

	case STAGE_WRITE_TO_CLIENT:
		n, err := unix.Write(sm.clientFd, sm.response)
		if err != nil {
			if err == unix.EAGAIN || err == unix.EWOULDBLOCK {
				return
			}

			fmt.Printf("Write error on fd %d: %v\n", sm.clientFd, err)
			sm.cleanup(allFds, states)
			return
		}

		if n > 0 {
			sm.response = sm.response[n:]
			fmt.Printf("Sent %d bytes to client, %d remaining\n", n, len(sm.response))
			if len(sm.response) == 0 {
				if _, exist := cache[sm.url]; !exist {
					cache[sm.url] = CacheEntry{Data: sm.fullResponse, Timestamp: time.Now()}
				}

				sm.cleanup(allFds, states)
			}
		}
	}
}
