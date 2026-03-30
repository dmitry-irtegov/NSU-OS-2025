package main

import (
	"bufio"
	"log"
	"net"

	"golang.org/x/sys/unix"
)

type Worker struct {
	id      int
	cache   *Cache
	newConn chan net.Conn
	wakeFd  [2]int
	byFd    map[int]*clientConn
}

func NewWorker(id int, cache *Cache) (*Worker, error) {
	var pipeFds [2]int
	if err := unix.Pipe(pipeFds[:]); err != nil {
		return nil, err
	}
	unix.SetNonblock(pipeFds[0], true)
	unix.SetNonblock(pipeFds[1], true)

	return &Worker{
		id:      id,
		cache:   cache,
		newConn: make(chan net.Conn, 256),
		wakeFd:  pipeFds,
		byFd:    make(map[int]*clientConn),
	}, nil
}

func (w *Worker) AddConn(conn net.Conn) {
	w.newConn <- conn
	unix.Write(w.wakeFd[1], []byte{1})
}

func (w *Worker) Run(quit <-chan struct{}) {
	for {
		select {
		case <-quit:
			w.closeAllConns()
			return
		default:
		}

		w.drainNewConns()

		fds := make([]unix.PollFd, 0, 1+len(w.byFd)*2)
		fds = append(fds, unix.PollFd{Fd: int32(w.wakeFd[0]), Events: unix.POLLIN})

		for fd, c := range w.byFd {
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
			if pfd.Events != 0 {
				fds = append(fds, pfd)
			}
		}

		n, err := unix.Poll(fds, 200)
		if err != nil {
			if err == unix.EINTR {
				continue
			}
			log.Printf("worker %d: poll error: %v", w.id, err)
			continue
		}
		if n == 0 {
			continue
		}

		for _, pfd := range fds {
			if pfd.Revents == 0 {
				continue
			}
			fd := int(pfd.Fd)

			if fd == w.wakeFd[0] {
				var buf [64]byte
				unix.Read(w.wakeFd[0], buf[:])
				w.drainNewConns()
				continue
			}

			c, ok := w.byFd[fd]
			if !ok {
				continue
			}

			if pfd.Revents&(unix.POLLHUP|unix.POLLERR|unix.POLLNVAL) != 0 {
				if fd == c.upFd && c.state == stateReadUpstream {
					w.finishUpstream(c)
					continue
				}
				w.closeAll(c)
				continue
			}

			switch c.state {
			case stateReadRequest:
				w.doReadRequest(c)
			case stateConnUpstream:
				w.doConnUpstream(c)
			case stateWriteUpstream:
				w.doWriteUpstream(c)
			case stateReadUpstream:
				w.doReadUpstream(c)
			case stateWriteClient:
				w.doWriteClient(c)
			}
		}
	}
}

func (w *Worker) drainNewConns() {
	for {
		select {
		case conn := <-w.newConn:
			tcpConn, ok := conn.(*net.TCPConn)
			if !ok {
				conn.Close()
				continue
			}
			cfd, err := connFd(tcpConn)
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
			w.byFd[cfd] = c
			log.Printf("worker %d: new connection from %s (fd=%d)", w.id, conn.RemoteAddr(), cfd)
		default:
			return
		}
	}
}

func (w *Worker) doReadRequest(c *clientConn) {
	req, err := parseRequest(c.reader)
	if err != nil {
		if isTemporary(err) {
			return
		}
		log.Printf("worker %d: fd=%d parse error: %v", w.id, c.fd, err)
		w.closeAll(c)
		return
	}

	log.Printf("worker %d: fd=%d %s %s", w.id, c.fd, req.Method, req.Target)
	c.req = req

	if req.Method != "GET" && req.Method != "HEAD" {
		w.sendToClient(c, errResponse(405, "Method Not Allowed"))
		return
	}

	if entry, hit := w.cache.Get(req.Target); hit {
		log.Printf("worker %d: fd=%d cache HIT for %s", w.id, c.fd, req.Target)
		w.sendToClient(c, entry.Data)
		return
	}

	log.Printf("worker %d: fd=%d cache MISS – connecting upstream for %s", w.id, c.fd, req.Target)
	if err := w.startUpstreamConnect(c); err != nil {
		log.Printf("worker %d: fd=%d upstream connect error: %v", w.id, c.fd, err)
		w.sendToClient(c, errResponse(502, "Bad Gateway"))
	}
}

func (w *Worker) startUpstreamConnect(c *clientConn) error {
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

	w.byFd[fd] = c
	return nil
}

func (w *Worker) doConnUpstream(c *clientConn) {
	nerr, err := unix.GetsockoptInt(c.upFd, unix.SOL_SOCKET, unix.SO_ERROR)
	if err != nil || nerr != 0 {
		log.Printf("worker %d: fd=%d upstream connect failed (soerr=%d)", w.id, c.fd, nerr)
		w.closeUpstream(c)
		w.sendToClient(c, errResponse(502, "Bad Gateway"))
		return
	}
	c.state = stateWriteUpstream
}

func (w *Worker) doWriteUpstream(c *clientConn) {
	remaining := c.upReqBytes[c.upSent:]
	n, err := unix.Write(c.upFd, remaining)
	if n > 0 {
		c.upSent += n
	}
	if err != nil && err != unix.EAGAIN && err != unix.EWOULDBLOCK {
		log.Printf("worker %d: fd=%d write upstream error: %v", w.id, c.fd, err)
		w.closeAll(c)
		return
	}
	if c.upSent >= len(c.upReqBytes) {
		c.state = stateReadUpstream
	}
}

func (w *Worker) doReadUpstream(c *clientConn) {
	buf := make([]byte, 4096)
	n, err := unix.Read(c.upFd, buf)
	if n > 0 {
		c.upBuf = append(c.upBuf, buf[:n]...)
	}
	if n == 0 || (err != nil && err != unix.EAGAIN && err != unix.EWOULDBLOCK) {
		w.finishUpstream(c)
	}
}

func (w *Worker) finishUpstream(c *clientConn) {
	w.closeUpstream(c)
	data := c.upBuf
	if len(data) == 0 {
		w.sendToClient(c, errResponse(502, "Bad Gateway"))
		return
	}
	w.cache.Set(c.req.Target, data)
	log.Printf("worker %d: fd=%d cached %d bytes for %s", w.id, c.fd, len(data), c.req.Target)
	w.sendToClient(c, data)
}

func (w *Worker) doWriteClient(c *clientConn) {
	remaining := c.response[c.sent:]
	n, err := unix.Write(c.fd, remaining)
	if n > 0 {
		c.sent += n
	}
	if err != nil && err != unix.EAGAIN && err != unix.EWOULDBLOCK {
		log.Printf("worker %d: fd=%d write client error: %v", w.id, c.fd, err)
		w.closeAll(c)
		return
	}
	if c.sent >= len(c.response) {
		log.Printf("worker %d: fd=%d response sent (%d bytes)", w.id, c.fd, c.sent)
		w.closeAll(c)
	}
}

func (w *Worker) sendToClient(c *clientConn, data []byte) {
	c.response = data
	c.sent = 0
	c.state = stateWriteClient
}

func (w *Worker) closeUpstream(c *clientConn) {
	if c.upFd >= 0 {
		delete(w.byFd, c.upFd)
		unix.Close(c.upFd)
		c.upFd = -1
	}
}

func (w *Worker) closeAll(c *clientConn) {
	w.closeUpstream(c)
	delete(w.byFd, c.fd)
	c.conn.Close()
}

func (w *Worker) closeAllConns() {
	for _, c := range w.byFd {
		if c.upFd >= 0 {
			unix.Close(c.upFd)
		}
		c.conn.Close()
	}
	w.byFd = make(map[int]*clientConn)
	unix.Close(w.wakeFd[0])
	unix.Close(w.wakeFd[1])
}
