package main

import (
	"bufio"
	"bytes"
	"fmt"
	"io/ioutil"
	"net"
	"strings"
)

type Request struct {
	Method  string
	Target  string
	Version string
	Headers []string
	Host    string
}

func parseRequest(r *bufio.Reader) (*Request, error) {

	line, err := readLine(r)
	if err != nil {
		return nil, fmt.Errorf("read request-line: %w", err)
	}
	parts := strings.SplitN(line, " ", 3)
	if len(parts) != 3 {
		return nil, fmt.Errorf("malformed request-line: %q", line)
	}
	req := &Request{
		Method:  parts[0],
		Target:  parts[1],
		Version: parts[2],
	}

	if req.Version != "HTTP/1.0" {
		return nil, fmt.Errorf("HTTP version not supported: %s", req.Version)
	}

	for {
		h, err := readLine(r)
		if err != nil {
			return nil, fmt.Errorf("read header: %w", err)
		}
		if h == "" {
			break
		}
		req.Headers = append(req.Headers, h)
		lower := strings.ToLower(h)
		if strings.HasPrefix(lower, "host:") {
			req.Host = strings.TrimSpace(h[5:])
		}
	}
	return req, nil
}

func buildUpstreamRequest(req *Request) []byte {
	var b bytes.Buffer
	fmt.Fprintf(&b, "%s %s HTTP/1.0\r\n", req.Method, req.Target)
	for _, h := range req.Headers {
		fmt.Fprintf(&b, "%s\r\n", h)
	}
	b.WriteString("Connection: close\r\n")
	b.WriteString("\r\n")
	return b.Bytes()
}

func upstreamAddr(req *Request) (string, error) {
	target := req.Target

	if idx := strings.Index(target, "://"); idx >= 0 {
		target = target[idx+3:]
	}

	if idx := strings.IndexByte(target, '/'); idx >= 0 {
		target = target[:idx]
	}

	if target == "" {
		target = req.Host
	}
	if target == "" {
		return "", fmt.Errorf("cannot determine upstream host from request")
	}

	if _, _, err := net.SplitHostPort(target); err != nil {
		target = target + ":80"
	}
	return target, nil
}

func fetchFromUpstream(req *Request) ([]byte, error) {
	addr, err := upstreamAddr(req)
	if err != nil {
		return nil, err
	}

	conn, err := net.Dial("tcp", addr)
	if err != nil {
		return nil, fmt.Errorf("dial %s: %w", addr, err)
	}
	defer conn.Close()

	if _, err = conn.Write(buildUpstreamRequest(req)); err != nil {
		return nil, fmt.Errorf("write upstream request: %w", err)
	}

	data, err := ioutil.ReadAll(conn)
	if err != nil {
		return nil, fmt.Errorf("read upstream response: %w", err)
	}
	return data, nil
}

func errResponse(code int, msg string) []byte {
	body := fmt.Sprintf("%d %s\n", code, msg)
	return []byte(fmt.Sprintf(
		"HTTP/1.0 %d %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
		code, msg, len(body), body,
	))
}

func readLine(r *bufio.Reader) (string, error) {
	line, err := r.ReadString('\n')
	line = strings.TrimRight(line, "\r\n")
	return line, err
}
