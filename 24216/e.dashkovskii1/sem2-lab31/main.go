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
	HeadersDone    bool
}

var (
	cache = make(map[string]*CacheEntry)
	conns = make(map[int]*Conn)
	upmap = make(map[int]*Conn)
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

func pollEINTR(fds []unix.PollFd, timeoutMs int) (int, error) {
	for {
		n, err := unix.Poll(fds, timeoutMs)
		if errors.Is(err, unix.EINTR) {
			continue
		}
		return n, err
	}
}

func connectEINTR(fd int, sa unix.Sockaddr) error {
	for {
		err := unix.Connect(fd, sa)
		if errors.Is(err, unix.EINTR) {
			continue
		}
		if errors.Is(err, unix.EINPROGRESS) {
			return nil
		}
		return err
	}
}

func makeListener(port int) (int, error) {
	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return -1, fmt.Errorf("socket failed")
	}
	unix.SetNonblock(fd, true)
	if err := unix.SetsockoptInt(fd, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1); err != nil {
		return -1, fmt.Errorf("socketopt failed")
	}
	if err := unix.Bind(fd, &unix.SockaddrInet4{Port: port, Addr: [4]byte{0, 0, 0, 0}}); err != nil {
		return -1, fmt.Errorf("bind failed")
	}
	if err := syscall.Listen(fd, 16); err != nil {
		return -1, fmt.Errorf("listen failed")
	}
	return fd, err
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
		return -1, fmt.Errorf("failed socket")
	}
	unix.SetNonblock(fd, true)

	if err := connectEINTR(fd, &unix.SockaddrInet4{Port: port, Addr: addr}); err != nil {
		unix.Close(fd)
		return -1, fmt.Errorf("failed connect")
	}
	return fd, nil
}

func handleNewClient(serverFd int) error {
	cfd, _, err := acceptEINTR(serverFd)
	if err != nil {
		return fmt.Errorf("accept failed")
	}
	unix.SetNonblock(cfd, false)

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
	unix.Shutdown(c.ClientFd, unix.SHUT_RDWR)
	unix.Close(c.ClientFd)
	if c.UpstreamFd >= 0 {
		unix.Shutdown(c.UpstreamFd, unix.SHUT_RDWR)
		unix.Close(c.UpstreamFd)
		delete(upmap, c.UpstreamFd)
	}
	delete(conns, clientFd)
}

func handleClientReadable(c *Conn) {
	buf := make([]byte, BufSize)
	n, err := readEINTR(c.ClientFd, buf)
	if err != nil {
		c.State = StateDone
		return
	}
	if n == 0 {
		c.State = StateDone
		return
	}
	c.Req = append(c.Req, buf[:n]...)
	if len(c.Req) > 32768 {
		c.State = StateDone
		return
	}

	if !bytes.Contains(c.Req, []byte("\r\n\r\n")) {
		return
	}

	idx := bytes.Index(c.Req, []byte("\r\n"))
	firstLine := string(c.Req[:idx])
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
	if host == "" {
		c.State = StateDone
		return
	}
	port := parsedUrl.Port()
	if port == "" {
		port = "80"
	}
	cacheKey := fmt.Sprintf("%s:%s%s", host, port, parsedUrl.RequestURI())

	if ce, ok := cache[cacheKey]; ok {
		c.SendData = ce.Data
		c.SendPtr = 0
		c.State = StateSendCache
		return
	}

	portInt, err := strconv.Atoi(port)
	if err != nil {
		c.State = StateDone
		return
	}

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

func upstreamFinish(c *Conn, success bool) {
	c.HeadersDone = true
	if success && len(c.RespBuf) > 0 && isCacheable(c.RespBuf) {
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
	n, err := readEINTR(c.UpstreamFd, buf)
	if err != nil {
		upstreamFinish(c, false)
		return
	}
	if n == 0 {
		upstreamFinish(c, true)
		return
	}
	c.RespBuf = append(c.RespBuf, buf[:n]...)
	if !c.HeadersDone && bytes.Contains(c.RespBuf, []byte("\r\n\r\n")) {
		c.RespBuf = forceConnectionClose(c.RespBuf)
		c.HeadersDone = true
	}
}

func handleUpstreamWritable(c *Conn) {
	if c.UpReqPtr == 0 && c.UpstreamFd >= 0 {
		soerr, err := unix.GetsockoptInt(c.UpstreamFd, unix.SOL_SOCKET, unix.SO_ERROR)
		if err != nil {
			upstreamFinish(c, false)
			return
		}
		if soerr != 0 {
			upstreamFinish(c, false)
			return
		}
		_ = unix.SetNonblock(c.UpstreamFd, false)
	}

	if c.UpReqPtr >= len(c.UpReq) {
		c.State = StateForward
		return
	}
	n, err := writeEINTR(c.UpstreamFd, c.UpReq[c.UpReqPtr:])
	if err != nil || n <= 0 {
		c.State = StateDone
		return
	}
	c.UpReqPtr += n
	if c.UpReqPtr >= len(c.UpReq) {
		unix.Shutdown(c.UpstreamFd, unix.SHUT_WR)
		c.State = StateForward
	}
}

func handleClientWritableForward(c *Conn) {
	if c.ClientWritePtr >= len(c.RespBuf) {
		if c.UpstreamEOF {
			unix.Shutdown(c.ClientFd, unix.SHUT_WR)
			c.State = StateDone
		}
		return
	}
	n, err := writeEINTR(c.ClientFd, c.RespBuf[c.ClientWritePtr:])
	if err != nil || n <= 0 {
		c.State = StateDone
		return
	}
	c.ClientWritePtr += n
	if c.UpstreamEOF && c.ClientWritePtr >= len(c.RespBuf) {
		unix.Shutdown(c.ClientFd, unix.SHUT_WR)
		c.State = StateDone
	}
}

func handleSendCache(c *Conn) {
	n, err := writeEINTR(c.ClientFd, c.SendData[c.SendPtr:])
	if err != nil || n <= 0 {
		c.State = StateDone
		return
	}
	c.SendPtr += n
	if c.SendPtr >= len(c.SendData) {
		unix.Shutdown(c.ClientFd, unix.SHUT_WR)
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
				if c.HeadersDone && c.ClientWritePtr < len(c.RespBuf) {
					fds = append(fds, unix.PollFd{Fd: int32(c.ClientFd), Events: unix.POLLOUT})
				}
				if c.UpstreamFd >= 0 {
					fds = append(fds, unix.PollFd{Fd: int32(c.UpstreamFd), Events: unix.POLLIN})
				}
			case StateSendCache:
				fds = append(fds, unix.PollFd{Fd: int32(c.ClientFd), Events: unix.POLLOUT})
			}
		}

		_, err := pollEINTR(fds, 1000)
		if err != nil {
			fmt.Fprintln(os.Stderr, "poll:", err)
			return
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
				if revents&unix.POLLIN != 0 && c.State == StateReadRequest {
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
				if revents&(unix.POLLERR|unix.POLLNVAL|unix.POLLHUP) != 0 {
					c.State = StateDone
				}
			} else if c, ok := upmap[fd]; ok {
				if revents&unix.POLLIN != 0 {
					handleUpstreamReadable(c)
				}
				if c.UpstreamFd == fd && revents&unix.POLLOUT != 0 {
					handleUpstreamWritable(c)
				}
				if c.UpstreamFd == fd && revents&(unix.POLLERR|unix.POLLHUP|unix.POLLNVAL) != 0 {
					if !c.UpstreamEOF {
						upstreamFinish(c, false)
					}
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
