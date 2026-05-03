package proxy

import (
	"fmt"
	"net"
	"strconv"
	"strings"
	"syscall"

	"golang.org/x/sys/unix"
)

type connState int

const (
	stateReading connState = iota
	stateConnecting
	stateForwarding
	stateSending
)

type fdSet struct{ unix.FdSet }

func (s *fdSet) zero() {
	for i := range s.Bits {
		s.Bits[i] = 0
	}
}

func (s *fdSet) set(fd int) {
	s.Bits[fd/64] |= 1 << (uint(fd) % 64)
}

func (s *fdSet) isSet(fd int) bool {
	return s.Bits[fd/64]&(1<<(uint(fd)%64)) != 0
}

type proxyConn struct {
	clientFd   int
	originFd   int
	state      connState
	reqBuf     []byte
	respBuf    []byte
	sendOff    int
	cacheKey   string
	host       string
	path       string
	originDone bool
}

func (c *proxyConn) close() {
	if c.clientFd >= 0 {
		unix.Shutdown(c.clientFd, unix.SHUT_RDWR)
		unix.Close(c.clientFd)
		c.clientFd = -1
	}
	if c.originFd >= 0 {
		unix.Shutdown(c.originFd, unix.SHUT_RDWR)
		unix.Close(c.originFd)
		c.originFd = -1
	}
}

func (c *proxyConn) headerEnd() int {
	for i := 0; i+3 < len(c.reqBuf); i++ {
		if c.reqBuf[i] == '\r' && c.reqBuf[i+1] == '\n' && c.reqBuf[i+2] == '\r' && c.reqBuf[i+3] == '\n' {
			return i
		}
	}
	return -1
}

type Proxy struct {
	listenFd int
	cache    *Cache
	conns    map[int]*proxyConn
}

func NewProxy(port int) (*Proxy, error) {
	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return nil, err
	}
	if err := unix.SetsockoptInt(fd, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1); err != nil {
		unix.Close(fd)
		return nil, err
	}
	addr := &unix.SockaddrInet4{Port: port}
	if err := unix.Bind(fd, addr); err != nil {
		unix.Close(fd)
		return nil, err
	}
	if err := syscall.Listen(fd, 128); err != nil {
		unix.Close(fd)
		return nil, err
	}
	return &Proxy{
		listenFd: fd,
		cache:    NewCache(),
		conns:    make(map[int]*proxyConn),
	}, nil
}

func (p *Proxy) Run() error {
	defer func() {
		unix.Shutdown(p.listenFd, unix.SHUT_RDWR)
		unix.Close(p.listenFd)
	}()

	for {
		fds := make([]unix.PollFd, 0, 1+len(p.conns)*2)
		fds = append(fds, unix.PollFd{Fd: int32(p.listenFd), Events: unix.POLLIN})

		for _, c := range p.conns {
			switch c.state {
			case stateReading:
				fds = append(fds, unix.PollFd{Fd: int32(c.clientFd), Events: unix.POLLIN})
			case stateConnecting:
				fds = append(fds, unix.PollFd{Fd: int32(c.originFd), Events: unix.POLLOUT})
			case stateForwarding:
				if !c.originDone {
					fds = append(fds, unix.PollFd{Fd: int32(c.originFd), Events: unix.POLLIN})
				}
				if c.sendOff < len(c.respBuf) {
					fds = append(fds, unix.PollFd{Fd: int32(c.clientFd), Events: unix.POLLOUT})
				}
			case stateSending:
				fds = append(fds, unix.PollFd{Fd: int32(c.clientFd), Events: unix.POLLOUT})
			}
		}

		if _, err := unix.Poll(fds, -1); err != nil {
			if err == unix.EINTR {
				continue
			}
			return err
		}

		var rset, wset fdSet
		rset.zero()
		wset.zero()
		errMask := int16(unix.POLLHUP | unix.POLLERR | unix.POLLNVAL)
		for _, pfd := range fds {
			fd := int(pfd.Fd)
			if pfd.Revents&(unix.POLLIN|errMask) != 0 {
				rset.set(fd)
			}
			if pfd.Revents&(unix.POLLOUT|errMask) != 0 {
				wset.set(fd)
			}
		}

		if rset.isSet(p.listenFd) {
			p.accept()
		}

		toRemove := make([]int, 0, 8)
		for fd, c := range p.conns {
			if p.handle(c, &rset, &wset) {
				toRemove = append(toRemove, fd)
			}
		}
		for _, fd := range toRemove {
			delete(p.conns, fd)
		}
	}
}

func (p *Proxy) accept() {
	fd, _, err := unix.Accept(p.listenFd)
	if err != nil {
		return
	}
	p.conns[fd] = &proxyConn{clientFd: fd, originFd: -1, state: stateReading}
}

func (p *Proxy) handle(c *proxyConn, rset, wset *fdSet) bool {
	switch c.state {
	case stateReading:
		return p.handleReading(c, rset)
	case stateConnecting:
		return p.handleConnecting(c, wset)
	case stateForwarding:
		return p.handleForwarding(c, rset, wset)
	case stateSending:
		return p.handleSending(c, wset)
	}
	return false
}

func (p *Proxy) handleReading(c *proxyConn, rset *fdSet) bool {
	if !rset.isSet(c.clientFd) {
		return false
	}
	var buf [4096]byte
	n, err := unix.Read(c.clientFd, buf[:])
	if n > 0 {
		c.reqBuf = append(c.reqBuf, buf[:n]...)
		if idx := c.headerEnd(); idx >= 0 {
			if e := p.startRequest(c, string(c.reqBuf[:idx])); e != nil {
				c.close()
				return true
			}
		}
	}
	if err != nil || n == 0 {
		c.close()
		return true
	}
	return false
}

func (p *Proxy) handleConnecting(c *proxyConn, wset *fdSet) bool {
	if !wset.isSet(c.originFd) {
		return false
	}
	soErr, err := unix.GetsockoptInt(c.originFd, unix.SOL_SOCKET, unix.SO_ERROR)
	if err != nil || soErr != 0 {
		c.close()
		return true
	}
	req := fmt.Sprintf("GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", c.path, c.host)
	if _, err := unix.Write(c.originFd, []byte(req)); err != nil {
		c.close()
		return true
	}
	c.state = stateForwarding
	return false
}

func (p *Proxy) handleForwarding(c *proxyConn, rset, wset *fdSet) bool {
	if !c.originDone && rset.isSet(c.originFd) {
		var buf [4096]byte
		n, err := unix.Read(c.originFd, buf[:])
		if n > 0 {
			c.respBuf = append(c.respBuf, buf[:n]...)
		}
		if err != nil || n == 0 {
			unix.Shutdown(c.originFd, unix.SHUT_RDWR)
			unix.Close(c.originFd)
			c.originFd = -1
			c.originDone = true
			if err == nil {
				p.cache.Set(c.cacheKey, c.respBuf)
			}
		}
	}
	if wset.isSet(c.clientFd) && c.sendOff < len(c.respBuf) {
		n, err := unix.Write(c.clientFd, c.respBuf[c.sendOff:])
		if n > 0 {
			c.sendOff += n
		}
		if err != nil {
			c.close()
			return true
		}
	}
	if c.originDone && c.sendOff >= len(c.respBuf) {
		c.close()
		return true
	}
	return false
}

func (p *Proxy) handleSending(c *proxyConn, wset *fdSet) bool {
	if c.sendOff >= len(c.respBuf) {
		c.close()
		return true
	}
	if !wset.isSet(c.clientFd) {
		return false
	}
	n, err := unix.Write(c.clientFd, c.respBuf[c.sendOff:])
	if n > 0 {
		c.sendOff += n
	}
	if err != nil || c.sendOff >= len(c.respBuf) {
		c.close()
		return true
	}
	return false
}

func (p *Proxy) startRequest(c *proxyConn, headers string) error {
	host, port, path, cacheKey, err := p.parseRequest(headers)
	if err != nil {
		return err
	}
	c.host = host
	c.path = path
	c.cacheKey = cacheKey

	if data, ok := p.cache.Get(cacheKey); ok {
		c.respBuf = data
		c.sendOff = 0
		c.state = stateSending
		return nil
	}

	ips, err := net.LookupIP(host)
	if err != nil {
		return err
	}

	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return err
	}
	if err := unix.SetNonblock(fd, true); err != nil {
		unix.Close(fd)
		return err
	}

	addr := &unix.SockaddrInet4{Port: port}
	copy(addr.Addr[:], ips[0].To4())

	err = unix.Connect(fd, addr)
	if err != nil && err != unix.EINPROGRESS {
		unix.Close(fd)
		return err
	}

	c.originFd = fd
	if err == nil {
		req := fmt.Sprintf("GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host)
		if _, err := unix.Write(fd, []byte(req)); err != nil {
			unix.Shutdown(fd, unix.SHUT_RDWR)
			unix.Close(fd)
			c.originFd = -1
			return err
		}
		c.state = stateForwarding
	} else {
		c.state = stateConnecting
	}
	return nil
}

func (p *Proxy) parseRequest(headers string) (host string, port int, path string, cacheKey string, err error) {
	lines := strings.SplitN(headers, "\r\n", 2)
	if len(lines) == 0 {
		return "", 0, "", "", fmt.Errorf("empty request")
	}
	parts := strings.SplitN(strings.TrimSpace(lines[0]), " ", 3)
	if len(parts) < 2 || parts[0] != "GET" {
		return "", 0, "", "", fmt.Errorf("only GET supported")
	}
	rawURL := parts[1]
	if !strings.HasPrefix(rawURL, "http://") {
		return "", 0, "", "", fmt.Errorf("only http:// URIs supported")
	}
	rest := rawURL[7:]
	slashIdx := strings.Index(rest, "/")
	var hostPort string
	if slashIdx < 0 {
		hostPort = rest
		path = "/"
	} else {
		hostPort = rest[:slashIdx]
		path = rest[slashIdx:]
		if path == "" {
			path = "/"
		}
	}
	port = 80
	if colonIdx := strings.LastIndex(hostPort, ":"); colonIdx >= 0 {
		host = hostPort[:colonIdx]
		port, err = strconv.Atoi(hostPort[colonIdx+1:])
		if err != nil {
			return "", 0, "", "", fmt.Errorf("invalid port")
		}
	} else {
		host = hostPort
	}
	if host == "" {
		return "", 0, "", "", fmt.Errorf("empty host")
	}
	cacheKey = fmt.Sprintf("http://%s:%d%s", host, port, path)
	return host, port, path, cacheKey, nil
}
