package main

import (
	"bytes"
	"fmt"
	"net"
	"net/url"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"

	"golang.org/x/sys/unix"
)

const (
	BufSize  = 8192
	MaxConns = 512
	Port     = 34543
)

type State int

const (
	StateReadRequest State = iota
	StateSendUpstreamReq
	StateForward
	StateSendCache
	StateDone
)

type CacheEntry struct {
	Data []byte
}

type Conn struct {
	ClientFd       int
	UpstreamFd     int
	State          State
	CacheKey       string
	Req            []byte
	UpReq          []byte
	UpReqPtr       int
	RespBuf        []byte
	SendPtr        int
	SendData       []byte
	ClientWritePtr int
	UpstreamEOF    bool
}

var (
	cache = make(map[string]*CacheEntry)
	conns = make(map[int]*Conn)
	upmap = make(map[int]*Conn)
)

func makeListener(port int) (int, error) {
	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return -1, fmt.Errorf("socket failed")
	}
	if err := unix.SetsockoptInt(fd, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1); err != nil {
		return -1, fmt.Errorf("socketopt failed")
	}
	if err := unix.Bind(fd, &unix.SockaddrInet4{Port: port, Addr: [4]byte{0, 0, 0, 0}}); err != nil {
		return -1, fmt.Errorf("bind failed")
	}
	if err := unix.Listen(fd, 16); err != nil {
		return -1, fmt.Errorf("listen failed")
	}
	return fd, err
}

func parseUrl(reqLine string) (string, string, *url.URL, error) {
	s := strings.Split(reqLine, " ")
	if len(s) < 3 {
		return "", "", nil, fmt.Errorf("split failed")
	}
	method := s[0]
	version := strings.TrimSpace(s[2])
	parsedUrl, err := url.Parse(s[1])
	if err != nil {
		return "", "", nil, fmt.Errorf("parse failed")
	}
	return method, version, parsedUrl, nil
}

func connectUpstream(host string, port int) (int, error) {
	ips, err := net.LookupHost(host)
	if err != nil {
		return -1, fmt.Errorf("dns resolve failed")
	}

	ip := net.ParseIP(ips[0]).To4()
	if ip == nil {
		return -1, fmt.Errorf("parseIP failed")
	}

	var addr [4]byte
	copy(addr[:], ip)

	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return -1, fmt.Errorf("failed socket")
	}

	if err := unix.Connect(fd, &unix.SockaddrInet4{Port: port, Addr: addr}); err != nil {
		unix.Close(fd)
		return -1, fmt.Errorf("failed connect")
	}
	return fd, nil
}

func handleNewClient(serverFd int) error {
	cfd, _, err := unix.Accept(serverFd)
	if err != nil {
		return fmt.Errorf("accept failed")
	}

	if len(conns) >= MaxConns {
		unix.Close(cfd)
		return nil
	}
	conns[cfd] = &Conn{ClientFd: cfd,
		UpstreamFd: -1,
		State:      StateReadRequest}
	return nil
}

func closeConn(clientFd int) {
	c, ok := conns[clientFd]
	if !ok {
		return
	}
	unix.Close(c.ClientFd)
	if c.UpstreamFd >= 0 {
		unix.Close(c.UpstreamFd)
		delete(upmap, c.UpstreamFd)
	}
	delete(conns, clientFd)
}

func handleClientReadable(c *Conn) {
	buf := make([]byte, BufSize)
	n, err := unix.Read(c.ClientFd, buf)
	if err != nil || n <= 0 {
		c.State = StateDone
		return
	}
	c.Req = append(c.Req, buf[:n]...)

	if !bytes.Contains(c.Req, []byte("\r\n\r\n")) {
		return
	}

	firstLine := strings.SplitN(string(c.Req), "\r\n", 2)[0]
	method, _, parsedUrl, err := parseUrl(firstLine)
	if err != nil {
		c.State = StateDone
		return
	}
	if method != "GET" {
		c.State = StateDone
		return
	}
	host := parsedUrl.Hostname()
	port := parsedUrl.Port()
	if port == "" {
		port = "80"
	}
	cacheKey := fmt.Sprintf("%s:%s%s", host, port, parsedUrl.Path)

	if ce, ok := cache[cacheKey]; ok {
		c.SendData = ce.Data
		c.SendPtr = 0
		c.State = StateSendCache
		return
	}
	portInt, _ := strconv.Atoi(port)
	ufd, err := connectUpstream(host, portInt)
	if err != nil {
		c.State = StateDone
		return
	}

	c.CacheKey = cacheKey
	c.UpstreamFd = ufd
	upmap[ufd] = c

	req := fmt.Sprintf("%s %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
		method, parsedUrl.RequestURI(), host)
	c.UpReq = []byte(req)
	c.UpReqPtr = 0
	c.State = StateSendUpstreamReq
}

func handleUpstreamWritable(c *Conn) {
	if c.UpReqPtr >= len(c.UpReq) {
		c.State = StateForward
		return
	}
	n, err := unix.Write(c.UpstreamFd, c.UpReq[c.UpReqPtr:])
	if n <= 0 || err != nil {
		c.State = StateDone
		return
	}
	c.UpReqPtr += n
	if c.UpReqPtr >= len(c.UpReq) {
		c.State = StateForward
	}
}

func upstreamFinish(c *Conn) {
	if len(c.RespBuf) > 0 {
		cache[c.CacheKey] = &CacheEntry{Data: c.RespBuf}
	}
	if c.UpstreamFd >= 0 {
		unix.Close(c.UpstreamFd)
		delete(upmap, c.UpstreamFd)
		c.UpstreamFd = -1
	}
	c.UpstreamEOF = true
	if c.ClientWritePtr >= len(c.RespBuf) {
		c.State = StateDone
	}
}

func handleUpstreamReadable(c *Conn) {
	buf := make([]byte, BufSize)
	n, err := unix.Read(c.UpstreamFd, buf)
	if n <= 0 || err != nil {
		upstreamFinish(c)
		return
	}
	c.RespBuf = append(c.RespBuf, buf[:n]...)
}

func handleClientWritableForward(c *Conn) {
	if c.ClientWritePtr >= len(c.RespBuf) {
		return
	}
	n, err := unix.Write(c.ClientFd, c.RespBuf[c.ClientWritePtr:])
	if n <= 0 || err != nil {
		c.State = StateDone
		return
	}
	c.ClientWritePtr += n
	if c.UpstreamEOF && c.ClientWritePtr >= len(c.RespBuf) {
		c.State = StateDone
	}
}

func handleSendCache(c *Conn) {
	n, err := unix.Write(c.ClientFd, c.SendData[c.SendPtr:])
	if n <= 0 || err != nil {
		c.State = StateDone
		return
	}
	c.SendPtr += n
	if c.SendPtr >= len(c.SendData) {
		c.State = StateDone
		return
	}
}

func main() {

	serverFd, err := makeListener(Port)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer unix.Close(serverFd)

	stop := make(chan os.Signal, 1)
	signal.Notify(stop, syscall.SIGINT)

	for {
		select {
		case <-stop:
			for fd := range conns {
				closeConn(fd)
			}
			return
		default:
		}

		fds := []unix.PollFd{
			{Fd: int32(serverFd), Events: unix.POLLIN},
		}
		for _, c := range conns {
			switch c.State {
			case StateReadRequest:
				fds = append(fds, unix.PollFd{Fd: int32(c.ClientFd), Events: unix.POLLIN})
			case StateSendUpstreamReq:
				if c.UpstreamFd >= 0 {
					fds = append(fds, unix.PollFd{Fd: int32(c.UpstreamFd), Events: unix.POLLOUT})
				}
			case StateForward:
				if c.ClientWritePtr < len(c.RespBuf) {
					fds = append(fds, unix.PollFd{Fd: int32(c.ClientFd), Events: unix.POLLOUT})
				}
				if c.UpstreamFd >= 0 {
					fds = append(fds, unix.PollFd{Fd: int32(c.UpstreamFd), Events: unix.POLLIN})
				}
			case StateSendCache:
				fds = append(fds, unix.PollFd{Fd: int32(c.ClientFd), Events: unix.POLLOUT})
			}
		}

		_, err := unix.Poll(fds, 1000)
		if err != nil {
			continue
		}

		if fds[0].Revents&unix.POLLIN != 0 {
			handleNewClient(serverFd)
		}

		for i := 1; i < len(fds); i++ {
			if fds[i].Revents == 0 {
				continue
			}
			fd := int(fds[i].Fd)
			revents := fds[i].Revents
			if c, ok := conns[fd]; ok {
				if revents&(unix.POLLERR|unix.POLLNVAL) != 0 {
					c.State = StateDone
					continue
				}
				if revents&(unix.POLLIN|unix.POLLHUP) != 0 && c.State == StateReadRequest {
					handleClientReadable(c)
				}
				if revents&unix.POLLOUT != 0 {
					switch c.State {
					case StateForward:
						handleClientWritableForward(c)
					case StateSendCache:
						handleSendCache(c)
					}
				}
			} else if c, ok := upmap[fd]; ok {
				if revents&(unix.POLLERR|unix.POLLNVAL) != 0 {
					upstreamFinish(c)
					continue
				}
				if revents&unix.POLLOUT != 0 && c.State == StateSendUpstreamReq {
					handleUpstreamWritable(c)
				}
				if revents&(unix.POLLIN|unix.POLLHUP) != 0 && c.State == StateForward {
					handleUpstreamReadable(c)
				}
			}
		}

		var dead []int
		for fd, c := range conns {
			if c.State == StateDone {
				dead = append(dead, fd)
			}
		}
		for _, fd := range dead {
			closeConn(fd)
		}
	}
}
