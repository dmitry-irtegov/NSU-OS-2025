package main

import (
	"bufio"
	"log"
	"net"
	"strings"

	"golang.org/x/sys/unix"
)

type connState int

const (
	stateReadRequest connState = iota
	stateConnUpstream
	stateWriteUpstream
	stateReadUpstream
	stateWriteClient
)

type clientConn struct {
	conn   net.Conn
	fd     int
	state  connState
	reader *bufio.Reader
	req    *Request

	upFd       int
	upReqBytes []byte
	upSent     int
	upBuf      []byte

	response []byte
	sent     int
}

type Proxy struct {
	addr     string
	cache    *Cache
	listener *net.TCPListener
	quit     chan struct{}
}

func NewProxy(addr string, cache *Cache) *Proxy {
	return &Proxy{addr: addr, cache: cache, quit: make(chan struct{})}
}

func (p *Proxy) Stop() {
	close(p.quit)
}

func (p *Proxy) Run() error {
	ln, err := net.Listen("tcp", p.addr)
	if err != nil {
		return err
	}
	p.listener = ln.(*net.TCPListener)
	defer p.listener.Close()

	rawLn, err := p.listener.SyscallConn()
	if err != nil {
		return err
	}
	var lnFd int
	rawLn.Control(func(fd uintptr) { lnFd = int(fd) })

	byFd := make(map[int]*clientConn)

	for {
		select {
		case <-p.quit:
			return nil
		default:
		}

		fds := make([]unix.PollFd, 0, 1+len(byFd))
		fds = append(fds, unix.PollFd{Fd: int32(lnFd), Events: unix.POLLIN})

		for fd, c := range byFd {
			pfd := unix.PollFd{Fd: int32(fd)}
			switch c.state {
			case stateReadRequest:
				if fd == c.fd {
					pfd.Events = unix.POLLIN
				}
			case stateConnUpstream, stateWriteUpstream:
				if fd == c.upFd {
					pfd.Events = unix.POLLOUT
				}
			case stateReadUpstream:
				if fd == c.upFd {
					pfd.Events = unix.POLLIN
				}
			case stateWriteClient:
				if fd == c.fd {
					pfd.Events = unix.POLLOUT
				}
			}
			fds = append(fds, pfd)
		}

		n, err := unix.Poll(fds, 200)
		if err != nil {
			if err == unix.EINTR {
				continue
			}
			return err
		}
		if n == 0 {
			continue
		}

		for _, pfd := range fds {
			if pfd.Revents == 0 {
				continue
			}
			fd := int(pfd.Fd)

			if fd == lnFd {
				conn, err := p.listener.AcceptTCP()
				if err != nil {
					log.Printf("accept: %v", err)
					continue
				}
				cfd, err := connFd(conn)
				if err != nil {
					conn.Close()
					continue
				}
				unix.SetNonblock(cfd, true)
				c := &clientConn{
					conn:   conn,
					fd:     cfd,
					state:  stateReadRequest,
					reader: bufio.NewReader(conn),
					upFd:   -1,
				}
				byFd[cfd] = c
				log.Printf("new connection from %s (fd=%d)", conn.RemoteAddr(), cfd)
				continue
			}

			c, ok := byFd[fd]
			if !ok {
				continue
			}

			if pfd.Revents&(unix.POLLHUP|unix.POLLERR|unix.POLLNVAL) != 0 {
				if fd == c.upFd && c.state == stateReadUpstream {
					p.finishUpstream(byFd, c)
					continue
				}
				p.closeAll(byFd, c)
				continue
			}

			switch c.state {
			case stateReadRequest:
				p.doReadRequest(byFd, c)
			case stateConnUpstream:
				p.doConnUpstream(byFd, c)
			case stateWriteUpstream:
				p.doWriteUpstream(byFd, c)
			case stateReadUpstream:
				p.doReadUpstream(byFd, c)
			case stateWriteClient:
				p.doWriteClient(byFd, c)
			}
		}
	}
}

func (p *Proxy) doReadRequest(byFd map[int]*clientConn, c *clientConn) {
	req, err := parseRequest(c.reader)
	if err != nil {
		if isTemporary(err) {
			return
		}
		log.Printf("fd=%d parse error: %v", c.fd, err)
		p.closeAll(byFd, c)
		return
	}

	log.Printf("fd=%d %s %s", c.fd, req.Method, req.Target)
	c.req = req

	if req.Method != "GET" && req.Method != "HEAD" {
		p.sendToClient(c, errResponse(405, "Method Not Allowed"))
		return
	}

	if entry, hit := p.cache.Get(req.Target); hit {
		log.Printf("fd=%d cache HIT for %s", c.fd, req.Target)
		p.sendToClient(c, entry.Data)
		return
	}

	log.Printf("fd=%d cache MISS – connecting upstream for %s", c.fd, req.Target)
	if err := p.startUpstreamConnect(byFd, c); err != nil {
		log.Printf("fd=%d upstream connect error: %v", c.fd, err)
		p.sendToClient(c, errResponse(502, "Bad Gateway"))
	}
}

func (p *Proxy) startUpstreamConnect(byFd map[int]*clientConn, c *clientConn) error {
	addr, err := upstreamAddr(c.req)
	if err != nil {
		return err
	}

	tcpAddr, err := net.ResolveTCPAddr("tcp", addr)
	if err != nil {
		return err
	}

	ip4 := tcpAddr.IP.To4()

	var fd int
	var sa unix.Sockaddr

	if ip4 != nil {
		fd, err = unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
		if err != nil {
			return err
		}
		s := &unix.SockaddrInet4{Port: tcpAddr.Port}
		copy(s.Addr[:], ip4)
		sa = s
	} else {
		fd, err = unix.Socket(unix.AF_INET6, unix.SOCK_STREAM, 0)
		if err != nil {
			return err
		}
		s := &unix.SockaddrInet6{Port: tcpAddr.Port}
		copy(s.Addr[:], tcpAddr.IP.To16())
		sa = s
	}

	unix.SetNonblock(fd, true)

	err = unix.Connect(fd, sa)
	if err != nil && err != unix.EINPROGRESS {
		unix.Close(fd)
		return err
	}

	c.upFd = fd
	c.upReqBytes = buildUpstreamRequest(c.req)
	c.upSent = 0
	c.upBuf = nil
	c.state = stateConnUpstream

	byFd[fd] = c
	return nil
}

func (p *Proxy) doConnUpstream(byFd map[int]*clientConn, c *clientConn) {
	nerr, err := unix.GetsockoptInt(c.upFd, unix.SOL_SOCKET, unix.SO_ERROR)
	if err != nil || nerr != 0 {
		log.Printf("fd=%d upstream connect failed (soerr=%d)", c.fd, nerr)
		p.closeUpstream(byFd, c)
		p.sendToClient(c, errResponse(502, "Bad Gateway"))
		return
	}
	c.state = stateWriteUpstream
}

func (p *Proxy) doWriteUpstream(byFd map[int]*clientConn, c *clientConn) {
	remaining := c.upReqBytes[c.upSent:]
	n, err := unix.Write(c.upFd, remaining)
	if n > 0 {
		c.upSent += n
	}
	if err != nil && err != unix.EAGAIN && err != unix.EWOULDBLOCK {
		log.Printf("fd=%d write upstream error: %v", c.fd, err)
		p.closeAll(byFd, c)
		return
	}
	if c.upSent >= len(c.upReqBytes) {
		c.state = stateReadUpstream
	}
}

func (p *Proxy) doReadUpstream(byFd map[int]*clientConn, c *clientConn) {
	buf := make([]byte, 4096)
	n, err := unix.Read(c.upFd, buf)
	if n > 0 {
		c.upBuf = append(c.upBuf, buf[:n]...)
	}
	if n == 0 || (err != nil && err != unix.EAGAIN && err != unix.EWOULDBLOCK) {
		p.finishUpstream(byFd, c)
	}
}

func (p *Proxy) finishUpstream(byFd map[int]*clientConn, c *clientConn) {
	p.closeUpstream(byFd, c)
	data := c.upBuf
	if len(data) == 0 {
		p.sendToClient(c, errResponse(502, "Bad Gateway"))
		return
	}
	p.cache.Set(c.req.Target, data)
	log.Printf("fd=%d cached %d bytes for %s", c.fd, len(data), c.req.Target)
	p.sendToClient(c, data)
}

func (p *Proxy) doWriteClient(byFd map[int]*clientConn, c *clientConn) {
	remaining := c.response[c.sent:]
	n, err := unix.Write(c.fd, remaining)
	if n > 0 {
		c.sent += n
	}
	if err != nil && err != unix.EAGAIN && err != unix.EWOULDBLOCK {
		log.Printf("fd=%d write client error: %v", c.fd, err)
		p.closeAll(byFd, c)
		return
	}
	if c.sent >= len(c.response) {
		log.Printf("fd=%d response sent (%d bytes)", c.fd, c.sent)
		p.closeAll(byFd, c)
	}
}

func (p *Proxy) sendToClient(c *clientConn, data []byte) {
	c.response = data
	c.sent = 0
	c.state = stateWriteClient
}

func (p *Proxy) closeUpstream(byFd map[int]*clientConn, c *clientConn) {
	if c.upFd >= 0 {
		delete(byFd, c.upFd)
		unix.Close(c.upFd)
		c.upFd = -1
	}
}

func (p *Proxy) closeAll(byFd map[int]*clientConn, c *clientConn) {
	p.closeUpstream(byFd, c)
	delete(byFd, c.fd)
	c.conn.Close()
}

func connFd(conn *net.TCPConn) (int, error) {
	raw, err := conn.SyscallConn()
	if err != nil {
		return 0, err
	}
	var fd int
	raw.Control(func(f uintptr) { fd = int(f) })
	return fd, nil
}

func isTemporary(err error) bool {
	if err == nil {
		return false
	}
	s := err.Error()
	return strings.Contains(s, "resource temporarily unavailable") ||
		strings.Contains(s, "try again") ||
		strings.Contains(s, "would block")
}
