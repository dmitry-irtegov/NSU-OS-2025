package main

import (
	"bytes"
	"errors"
	"fmt"
	"net"
	"net/url"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"

	"golang.org/x/sys/unix"
)

const (
	BufSize       = 8192
	Port          = 34543
	MaxReqSize    = 32768
	ioTimeout     = 30 * time.Second
	shutdownGrace = 5 * time.Second
	pollTimeoutMs = 1000
)

type State int

const (
	StateReadRequest State = iota
	StateForward
	StateSendCache
	StateDone
)

type Conn struct {
	clientFd    int
	upstreamFd  int
	state       State
	reqBuf      []byte
	cacheKey    string
	respBuf     []byte
	headSent    bool
	clientAlive bool
	sendBuf     []byte
	sendOff     int
}

func (c *Conn) close() {
	if c.clientFd >= 0 {
		unix.Close(c.clientFd)
		c.clientFd = -1
	}
	if c.upstreamFd >= 0 {
		unix.Close(c.upstreamFd)
		c.upstreamFd = -1
	}
	c.state = StateDone
}

var (
	cache   = make(map[string][]byte)
	cacheMu sync.RWMutex
)

func readEINTR(fd int, buf []byte) (int, error) {
	for {
		n, err := unix.Read(fd, buf)
		if errors.Is(err, unix.EINTR) {
			continue
		}
		return n, err
	}
}

func writeEINTR(fd int, buf []byte) (int, error) {
	for {
		n, err := unix.Write(fd, buf)
		if errors.Is(err, unix.EINTR) {
			continue
		}
		return n, err
	}
}

func acceptEINTR(fd int) (int, unix.Sockaddr, error) {
	for {
		nfd, sa, err := unix.Accept(fd)
		if errors.Is(err, unix.EINTR) {
			continue
		}
		return nfd, sa, err
	}
}

func connectEINTR(fd int, sa unix.Sockaddr) error {
	for {
		err := unix.Connect(fd, sa)
		if errors.Is(err, unix.EINTR) {
			continue
		}
		return err
	}
}

func pollEINTR(fds []unix.PollFd, timeoutMs int) (int, error) {
	for {
		n, err := unix.Poll(fds, timeoutMs)
		if errors.Is(err, unix.EINTR) {
			continue
		}
		return n, err
	}
}

func setSockTimeouts(fd int, d time.Duration) {
	tv := unix.NsecToTimeval(d.Nanoseconds())
	_ = unix.SetsockoptTimeval(fd, unix.SOL_SOCKET, unix.SO_RCVTIMEO, &tv)
	_ = unix.SetsockoptTimeval(fd, unix.SOL_SOCKET, unix.SO_SNDTIMEO, &tv)
}

func writeAllFd(fd int, buf []byte) error {
	for len(buf) > 0 {
		n, err := writeEINTR(fd, buf)
		if err != nil {
			return err
		}
		if n <= 0 {
			return fmt.Errorf("short write")
		}
		buf = buf[n:]
	}
	return nil
}

func parseUrl(reqLine string) (string, string, *url.URL, error) {
	s := strings.Fields(reqLine)
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
	if err != nil || len(ips) == 0 {
		return -1, fmt.Errorf("dns resolve failed")
	}

	var ip net.IP
	for _, ipStr := range ips {
		if parsed := net.ParseIP(ipStr).To4(); parsed != nil {
			ip = parsed
			break
		}
	}
	if ip == nil {
		return -1, fmt.Errorf("no ipv4 address found")
	}

	var addr [4]byte
	copy(addr[:], ip)

	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return -1, fmt.Errorf("socket failed")
	}
	if err := connectEINTR(fd, &unix.SockaddrInet4{Port: port, Addr: addr}); err != nil {
		unix.Close(fd)
		return -1, fmt.Errorf("connect failed")
	}
	return fd, nil
}

func makeListener(port int) (int, error) {
	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return -1, fmt.Errorf("socket failed")
	}
	if err := unix.SetNonblock(fd, true); err != nil {
		return -1, fmt.Errorf("nonblock failed")
	}
	if err := unix.SetsockoptInt(fd, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1); err != nil {
		return -1, fmt.Errorf("setsockopt failed")
	}
	if err := unix.Bind(fd, &unix.SockaddrInet4{Port: port, Addr: [4]byte{0, 0, 0, 0}}); err != nil {
		return -1, fmt.Errorf("bind failed")
	}
	if err := syscall.Listen(fd, 64); err != nil {
		return -1, fmt.Errorf("listen failed")
	}
	return fd, nil
}

func forceConnectionClose(resp []byte) []byte {
	headerEnd := bytes.Index(resp, []byte("\r\n\r\n"))
	if headerEnd < 0 {
		return resp
	}
	header := resp[:headerEnd]
	body := resp[headerEnd+4:]

	lines := strings.Split(string(header), "\r\n")
	out := make([]string, 0, len(lines)+1)
	out = append(out, lines[0])
	out = append(out, "Connection: close")
	for _, l := range lines[1:] {
		lower := strings.ToLower(l)
		if strings.HasPrefix(lower, "connection:") ||
			strings.HasPrefix(lower, "proxy-connection:") ||
			strings.HasPrefix(lower, "keep-alive:") {
			continue
		}
		out = append(out, l)
	}
	newHeader := strings.Join(out, "\r\n")

	result := make([]byte, 0, len(newHeader)+4+len(body))
	result = append(result, newHeader...)
	result = append(result, "\r\n\r\n"...)
	result = append(result, body...)
	return result
}

func isCacheable(resp []byte) bool {
	headerEnd := bytes.Index(resp, []byte("\r\n\r\n"))
	if headerEnd < 0 {
		return false
	}
	header := resp[:headerEnd]
	body := resp[headerEnd+4:]

	statusEnd := bytes.Index(header, []byte("\r\n"))
	if statusEnd < 0 {
		statusEnd = len(header)
	}
	statusLine := string(header[:statusEnd])
	parts := strings.Fields(statusLine)
	if len(parts) < 2 || parts[1] != "200" {
		return false
	}

	lines := bytes.Split(header, []byte("\r\n"))
	for _, ln := range lines {
		i := bytes.IndexByte(ln, ':')
		if i < 0 {
			continue
		}
		name := strings.ToLower(strings.TrimSpace(string(ln[:i])))
		val := strings.TrimSpace(string(ln[i+1:]))
		if name == "transfer-encoding" && strings.EqualFold(val, "chunked") {
			return false
		}
		if name == "content-length" {
			cl, err := strconv.Atoi(val)
			if err != nil || cl < 0 {
				return false
			}
			if len(body) != cl {
				return false
			}
		}
	}
	return true
}

type Worker struct {
	id       int
	wakeR    int
	wakeW    int
	mu       sync.Mutex
	incoming []int
	conns    map[int]*Conn
}

func newWorker(id int) (*Worker, error) {
	var p [2]int
	if err := unix.Pipe(p[:]); err != nil {
		return nil, fmt.Errorf("pipe failed")
	}
	if err := unix.SetNonblock(p[1], true); err != nil {
		unix.Close(p[0])
		unix.Close(p[1])
		return nil, fmt.Errorf("nonblock failed")
	}
	return &Worker{
		id:    id,
		wakeR: p[0],
		wakeW: p[1],
		conns: make(map[int]*Conn),
	}, nil
}

func (w *Worker) submit(cfd int) {
	w.mu.Lock()
	w.incoming = append(w.incoming, cfd)
	w.mu.Unlock()
	_, _ = unix.Write(w.wakeW, []byte{0})
}

func (w *Worker) drainWake() {
	var buf [256]byte
	for {
		n, err := unix.Read(w.wakeR, buf[:])
		if err != nil || n < len(buf) {
			return
		}
	}
}

func (w *Worker) pullIncoming() {
	w.mu.Lock()
	newConns := w.incoming
	w.incoming = nil
	w.mu.Unlock()
	for _, cfd := range newConns {
		setSockTimeouts(cfd, ioTimeout)
		c := &Conn{
			clientFd:    cfd,
			upstreamFd:  -1,
			state:       StateReadRequest,
			clientAlive: true,
		}
		w.conns[cfd] = c
	}
}

func (w *Worker) handleClientReadable(c *Conn) {
	buf := make([]byte, BufSize)
	n, err := readEINTR(c.clientFd, buf)
	if n <= 0 || err != nil {
		c.state = StateDone
		return
	}
	c.reqBuf = append(c.reqBuf, buf[:n]...)
	if len(c.reqBuf) > MaxReqSize {
		c.state = StateDone
		return
	}
	if !bytes.Contains(c.reqBuf, []byte("\r\n\r\n")) {
		return
	}

	idx := bytes.Index(c.reqBuf, []byte("\r\n"))
	method, _, parsedUrl, err := parseUrl(string(c.reqBuf[:idx]))
	if err != nil || method != "GET" {
		c.state = StateDone
		return
	}
	host := parsedUrl.Hostname()
	if host == "" {
		c.state = StateDone
		return
	}
	port := parsedUrl.Port()
	if port == "" {
		port = "80"
	}
	portInt, err := strconv.Atoi(port)
	if err != nil {
		c.state = StateDone
		return
	}
	c.cacheKey = fmt.Sprintf("%s:%s%s", host, port, parsedUrl.RequestURI())

	cacheMu.RLock()
	data, hit := cache[c.cacheKey]
	cacheMu.RUnlock()
	if hit {
		c.sendBuf = data
		c.sendOff = 0
		c.state = StateSendCache
		return
	}

	ufd, err := connectUpstream(host, portInt)
	if err != nil {
		c.state = StateDone
		return
	}
	setSockTimeouts(ufd, ioTimeout)
	c.upstreamFd = ufd
	w.conns[ufd] = c

	upReq := fmt.Sprintf("%s %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
		method, parsedUrl.RequestURI(), host)
	if err := writeAllFd(ufd, []byte(upReq)); err != nil {
		c.state = StateDone
		return
	}
	unix.Shutdown(ufd, unix.SHUT_WR)
	c.state = StateForward
}

func (w *Worker) handleUpstreamReadable(c *Conn) {
	buf := make([]byte, BufSize)
	n, err := readEINTR(c.upstreamFd, buf)
	if n > 0 {
		c.respBuf = append(c.respBuf, buf[:n]...)
		if !c.headSent {
			if bytes.Contains(c.respBuf, []byte("\r\n\r\n")) {
				c.respBuf = forceConnectionClose(c.respBuf)
				if c.clientAlive && writeAllFd(c.clientFd, c.respBuf) != nil {
					c.clientAlive = false
				}
				c.headSent = true
			}
		} else if c.clientAlive {
			if writeAllFd(c.clientFd, buf[:n]) != nil {
				c.clientAlive = false
			}
		}
	}
	if err != nil {
		c.state = StateDone
		return
	}
	if n == 0 {
		if c.clientAlive {
			unix.Shutdown(c.clientFd, unix.SHUT_WR)
		}
		if c.headSent && isCacheable(c.respBuf) {
			cacheMu.Lock()
			cache[c.cacheKey] = c.respBuf
			cacheMu.Unlock()
		}
		c.state = StateDone
	}
}

func (w *Worker) handleSendCache(c *Conn) {
	if c.sendOff >= len(c.sendBuf) {
		unix.Shutdown(c.clientFd, unix.SHUT_WR)
		c.state = StateDone
		return
	}
	n, err := writeEINTR(c.clientFd, c.sendBuf[c.sendOff:])
	if err != nil || n <= 0 {
		c.state = StateDone
		return
	}
	c.sendOff += n
	if c.sendOff >= len(c.sendBuf) {
		unix.Shutdown(c.clientFd, unix.SHUT_WR)
		c.state = StateDone
	}
}

func (w *Worker) closeAll() {
	for _, c := range w.conns {
		c.close()
	}
	w.conns = make(map[int]*Conn)
}

func (w *Worker) run(wg *sync.WaitGroup, stop <-chan struct{}) {
	defer wg.Done()
	defer unix.Close(w.wakeR)
	defer unix.Close(w.wakeW)

	for {
		select {
		case <-stop:
			w.closeAll()
			return
		default:
		}

		fds := make([]unix.PollFd, 0, len(w.conns)+1)
		fds = append(fds, unix.PollFd{Fd: int32(w.wakeR), Events: unix.POLLIN})
		for fd, c := range w.conns {
			var ev int16
			switch c.state {
			case StateReadRequest:
				if fd == c.clientFd {
					ev = unix.POLLIN
				}
			case StateForward:
				if fd == c.upstreamFd {
					ev = unix.POLLIN
				}
			case StateSendCache:
				if fd == c.clientFd {
					ev = unix.POLLOUT
				}
			}
			if ev != 0 {
				fds = append(fds, unix.PollFd{Fd: int32(fd), Events: ev})
			}
		}

		if _, err := pollEINTR(fds, pollTimeoutMs); err != nil {
			continue
		}

		if fds[0].Revents&unix.POLLIN != 0 {
			w.drainWake()
			w.pullIncoming()
		}

		var toClose []*Conn
		for i := 1; i < len(fds); i++ {
			rev := fds[i].Revents
			if rev == 0 {
				continue
			}
			fd := int(fds[i].Fd)
			c, ok := w.conns[fd]
			if !ok || c.state == StateDone {
				continue
			}
			switch c.state {
			case StateReadRequest:
				if rev&(unix.POLLIN|unix.POLLHUP|unix.POLLERR) != 0 {
					w.handleClientReadable(c)
				}
			case StateForward:
				if fd == c.upstreamFd && rev&(unix.POLLIN|unix.POLLHUP|unix.POLLERR) != 0 {
					w.handleUpstreamReadable(c)
				}
			case StateSendCache:
				if fd == c.clientFd && rev&(unix.POLLOUT|unix.POLLERR) != 0 {
					w.handleSendCache(c)
				}
			}
			if c.state == StateDone {
				toClose = append(toClose, c)
			}
		}
		for _, c := range toClose {
			cfd, ufd := c.clientFd, c.upstreamFd
			c.close()
			if cfd >= 0 {
				delete(w.conns, cfd)
			}
			if ufd >= 0 {
				delete(w.conns, ufd)
			}
		}
	}
}

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, "usage: proxy <pool_size>")
		os.Exit(1)
	}
	poolSize, err := strconv.Atoi(os.Args[1])
	if err != nil || poolSize <= 0 {
		fmt.Fprintln(os.Stderr, "invalid pool size")
		os.Exit(1)
	}

	serverFd, err := makeListener(Port)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer unix.Close(serverFd)

	stop := make(chan struct{})
	sig := make(chan os.Signal, 1)
	signal.Notify(sig, syscall.SIGINT)

	workers := make([]*Worker, poolSize)
	var wg sync.WaitGroup
	for i := 0; i < poolSize; i++ {
		wk, err := newWorker(i)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
		workers[i] = wk
		wg.Add(1)
		go wk.run(&wg, stop)
	}

	acceptDone := make(chan struct{})
	go func() {
		defer close(acceptDone)
		nextWorker := 0
		fds := []unix.PollFd{{Fd: int32(serverFd), Events: unix.POLLIN}}
		for {
			select {
			case <-stop:
				return
			default:
			}
			fds[0].Revents = 0
			if _, err := pollEINTR(fds, pollTimeoutMs); err != nil {
				continue
			}
			if fds[0].Revents&unix.POLLIN == 0 {
				continue
			}
			cfd, _, err := acceptEINTR(serverFd)
			if err != nil {
				continue
			}
			if err := unix.SetNonblock(cfd, false); err != nil {
				unix.Close(cfd)
				continue
			}
			i := nextWorker % poolSize
			nextWorker++
			workers[i].submit(cfd)
		}
	}()

	<-sig
	close(stop)
	<-acceptDone

	done := make(chan struct{})
	go func() { wg.Wait(); close(done) }()
	select {
	case <-done:
	case <-time.After(shutdownGrace):
	}
}
