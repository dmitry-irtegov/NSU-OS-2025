package proxy

import (
	"fmt"
	"net"
	"strconv"
	"strings"
	"sync"

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

func (s *fdSet) setMax(fd int, maxFd *int) {
	s.set(fd)
	if fd > *maxFd {
		*maxFd = fd
	}
}

type workerConn struct {
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

func (c *workerConn) close() {
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

func (c *workerConn) headerEnd() int {
	for i := 0; i+3 < len(c.reqBuf); i++ {
		if c.reqBuf[i] == '\r' && c.reqBuf[i+1] == '\n' && c.reqBuf[i+2] == '\r' && c.reqBuf[i+3] == '\n' {
			return i
		}
	}
	return -1
}

type Worker struct {
	pipeR int
	pipeW int
	mu    sync.Mutex
	queue []*workerConn
	conns map[int]*workerConn
	cache *Cache
}

func newWorker(cache *Cache) (*Worker, error) {
	fds := make([]int, 2)
	if err := unix.Pipe(fds); err != nil {
		return nil, err
	}
	return &Worker{
		pipeR: fds[0],
		pipeW: fds[1],
		conns: make(map[int]*workerConn),
		cache: cache,
	}, nil
}

func (w *Worker) assign(clientFd int) {
	c := &workerConn{clientFd: clientFd, originFd: -1, state: stateReading}
	w.mu.Lock()
	w.queue = append(w.queue, c)
	w.mu.Unlock()
	unix.Write(w.pipeW, []byte{0})
}

func (w *Worker) run() {
	for {
		var rset, wset fdSet
		rset.zero()
		wset.zero()
		maxFd := w.pipeR
		rset.set(w.pipeR)

		for _, c := range w.conns {
			switch c.state {
			case stateReading:
				rset.setMax(c.clientFd, &maxFd)
			case stateConnecting:
				wset.setMax(c.originFd, &maxFd)
			case stateForwarding:
				if !c.originDone {
					rset.setMax(c.originFd, &maxFd)
				}
				if c.sendOff < len(c.respBuf) {
					wset.setMax(c.clientFd, &maxFd)
				}
			case stateSending:
				wset.setMax(c.clientFd, &maxFd)
			}
		}

		if _, err := unix.Select(maxFd+1, &rset.FdSet, &wset.FdSet, nil, nil); err != nil {
			if err == unix.EINTR {
				continue
			}
			return
		}

		if rset.isSet(w.pipeR) {
			var buf [64]byte
			unix.Read(w.pipeR, buf[:])
			w.mu.Lock()
			for _, c := range w.queue {
				w.conns[c.clientFd] = c
			}
			w.queue = nil
			w.mu.Unlock()
		}

		var toRemove []int
		for fd, c := range w.conns {
			if w.handle(c, &rset, &wset) {
				toRemove = append(toRemove, fd)
			}
		}
		for _, fd := range toRemove {
			delete(w.conns, fd)
		}
	}
}

func (w *Worker) handle(c *workerConn, rset, wset *fdSet) bool {
	switch c.state {
	case stateReading:
		return w.handleReading(c, rset)
	case stateConnecting:
		return w.handleConnecting(c, wset)
	case stateForwarding:
		return w.handleForwarding(c, rset, wset)
	case stateSending:
		return w.handleSending(c, wset)
	}
	return false
}

func (w *Worker) handleReading(c *workerConn, rset *fdSet) bool {
	if !rset.isSet(c.clientFd) {
		return false
	}
	var buf [4096]byte
	n, err := unix.Read(c.clientFd, buf[:])
	if n > 0 {
		c.reqBuf = append(c.reqBuf, buf[:n]...)
		if idx := c.headerEnd(); idx >= 0 {
			if e := w.startRequest(c, string(c.reqBuf[:idx])); e != nil {
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

func (w *Worker) handleConnecting(c *workerConn, wset *fdSet) bool {
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

func (w *Worker) handleForwarding(c *workerConn, rset, wset *fdSet) bool {
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
			w.cache.Set(c.cacheKey, c.respBuf)
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

func (w *Worker) handleSending(c *workerConn, wset *fdSet) bool {
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

func (w *Worker) startRequest(c *workerConn, headers string) error {
	host, port, path, cacheKey, err := w.parseRequest(headers)
	if err != nil {
		return err
	}
	c.host = host
	c.path = path
	c.cacheKey = cacheKey

	if data, ok := w.cache.Get(cacheKey); ok {
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

func (w *Worker) parseRequest(headers string) (host string, port int, path string, cacheKey string, err error) {
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
