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
	MaxConns      = 512
	ioTimeout     = 30 * time.Second
	shutdownGrace = 5 * time.Second
)

var (
	cache   = make(map[string][]byte)
	cacheMu sync.RWMutex
	sem     = make(chan struct{}, MaxConns)
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

func makeListener(port int) (int, error) {
	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return -1, fmt.Errorf("socket failed")
	}
	unix.SetNonblock(fd, true)
	if err := unix.SetsockoptInt(fd, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1); err != nil {
		return -1, fmt.Errorf("setsockopt failed")
	}
	if err := unix.Bind(fd, &unix.SockaddrInet4{Port: port, Addr: [4]byte{0, 0, 0, 0}}); err != nil {
		return -1, fmt.Errorf("bind failed")
	}
	if err := syscall.Listen(fd, 16); err != nil {
		return -1, fmt.Errorf("listen failed")
	}
	return fd, nil
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

func readRequest(fd int) ([]byte, error) {
	var req []byte
	buf := make([]byte, BufSize)
	for {
		n, err := readEINTR(fd, buf)
		if n > 0 {
			req = append(req, buf[:n]...)
			if bytes.Contains(req, []byte("\r\n\r\n")) {
				return req, nil
			}
			if len(req) > 32768 {
				return nil, fmt.Errorf("request too large")
			}
		}
		if err != nil {
			return nil, err
		}
		if n == 0 {
			return nil, fmt.Errorf("eof before headers")
		}
	}
}

func readResponse(fd int) ([]byte, bool) {
	var resp []byte
	buf := make([]byte, BufSize)
	for {
		n, err := readEINTR(fd, buf)
		if n > 0 {
			resp = append(resp, buf[:n]...)
		}
		if err != nil {
			return resp, false
		}
		if n == 0 {
			return resp, true
		}
	}
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

func handleConn(clientFd int) {
	defer unix.Close(clientFd)
	setSockTimeouts(clientFd, ioTimeout)

	req, err := readRequest(clientFd)
	if err != nil {
		return
	}

	idx := bytes.Index(req, []byte("\r\n"))
	method, _, parsedUrl, err := parseUrl(string(req[:idx]))
	if err != nil || method != "GET" {
		return
	}
	host := parsedUrl.Hostname()
	if host == "" {
		return
	}
	port := parsedUrl.Port()
	if port == "" {
		port = "80"
	}
	cacheKey := fmt.Sprintf("%s:%s%s", host, port, parsedUrl.RequestURI())

	cacheMu.RLock()
	data, hit := cache[cacheKey]
	cacheMu.RUnlock()
	if hit {
		if writeAllFd(clientFd, data) == nil {
			unix.Shutdown(clientFd, unix.SHUT_WR)
		}
		return
	}

	portInt, err := strconv.Atoi(port)
	if err != nil {
		return
	}
	ufd, err := connectUpstream(host, portInt)
	if err != nil {
		return
	}
	defer unix.Close(ufd)
	setSockTimeouts(ufd, ioTimeout)

	upReq := fmt.Sprintf("%s %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
		method, parsedUrl.RequestURI(), host)
	if err := writeAllFd(ufd, []byte(upReq)); err != nil {
		return
	}
	unix.Shutdown(ufd, unix.SHUT_WR)

	resp, ok := readResponse(ufd)
	if !ok {
		if len(resp) > 0 {
			_ = writeAllFd(clientFd, resp)
		}
		return
	}

	out := forceConnectionClose(resp)
	if writeAllFd(clientFd, out) == nil {
		unix.Shutdown(clientFd, unix.SHUT_WR)
	}

	if isCacheable(resp) {
		cacheMu.Lock()
		cache[cacheKey] = out
		cacheMu.Unlock()
	}
}

func drain(wg *sync.WaitGroup) {
	done := make(chan struct{})
	go func() {
		wg.Wait()
		close(done)
	}()
	select {
	case <-done:
	case <-time.After(shutdownGrace):
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

	var wg sync.WaitGroup
	for {
		select {
		case <-stop:
			drain(&wg)
			return
		default:
		}

		fds := []unix.PollFd{{Fd: int32(serverFd), Events: unix.POLLIN}}
		n, err := pollEINTR(fds, 1000)
		if err != nil {
			fmt.Fprintln(os.Stderr, "poll:", err)
			return
		}
		if n == 0 || fds[0].Revents&unix.POLLIN == 0 {
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

		sem <- struct{}{}
		wg.Add(1)
		go func(fd int) {
			defer wg.Done()
			defer func() { <-sem }()
			handleConn(fd)
		}(cfd)
	}
}
