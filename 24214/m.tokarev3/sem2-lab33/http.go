package main

import (
	"bufio"
	"bytes"
	"fmt"
	"io"
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
	Body    []byte
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

	contentLength := -1
	isChunked := false

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
		} else if strings.HasPrefix(lower, "content-length:") {
			fmt.Sscanf(lower, "content-length: %d", &contentLength)
		} else if strings.HasPrefix(lower, "transfer-encoding:") && strings.Contains(lower, "chunked") {
			isChunked = true
		}
	}

	var bodyBuf bytes.Buffer
	if isChunked {
		for {
			chunkHeader, err := readLine(r)
			if err != nil {
				return nil, err
			}
			bodyBuf.WriteString(chunkHeader + "\r\n")

			var chunkSize int
			fmt.Sscanf(chunkHeader, "%x", &chunkSize)

			if chunkSize == 0 {
				empty, _ := readLine(r)
				bodyBuf.WriteString(empty + "\r\n")
				break
			}

			chunkData := make([]byte, chunkSize)
			if _, err := io.ReadFull(r, chunkData); err != nil {
				return nil, err
			}
			bodyBuf.Write(chunkData)

			crlf, _ := readLine(r)
			bodyBuf.WriteString(crlf + "\r\n")
		}
	} else if contentLength > 0 {
		body := make([]byte, contentLength)
		if _, err := io.ReadFull(r, body); err != nil {
			return nil, err
		}
		bodyBuf.Write(body)
	}
	req.Body = bodyBuf.Bytes()

	return req, nil
}

func buildUpstreamRequest(req *Request) []byte {
	var b bytes.Buffer
	fmt.Fprintf(&b, "%s %s %s\r\n", req.Method, req.Target, req.Version)
	for _, h := range req.Headers {
		fmt.Fprintf(&b, "%s\r\n", h)
	}
	b.WriteString("Connection: close\r\n")
	b.WriteString("\r\n")
	if len(req.Body) > 0 {
		b.Write(req.Body)
	}
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

func readUpstreamResponse(r *bufio.Reader) ([]byte, error) {
	statusLine, err := readLine(r)
	if err != nil {
		return nil, err
	}

	var buf bytes.Buffer
	buf.WriteString(statusLine + "\r\n")

	contentLength := -1
	isChunked := false

	for {
		header, err := readLine(r)
		if err != nil {
			return nil, err
		}
		buf.WriteString(header + "\r\n")

		if header == "" {
			break
		}

		lower := strings.ToLower(header)
		if strings.HasPrefix(lower, "content-length:") {
			fmt.Sscanf(lower, "content-length: %d", &contentLength)
		} else if strings.HasPrefix(lower, "transfer-encoding:") && strings.Contains(lower, "chunked") {
			isChunked = true
		}
	}

	if isChunked {
		for {
			chunkHeader, err := readLine(r)
			if err != nil {
				return nil, err
			}
			buf.WriteString(chunkHeader + "\r\n")

			var chunkSize int
			fmt.Sscanf(chunkHeader, "%x", &chunkSize)

			if chunkSize == 0 {
				empty, _ := readLine(r)
				buf.WriteString(empty + "\r\n")
				break
			}

			chunkData := make([]byte, chunkSize)
			if _, err := io.ReadFull(r, chunkData); err != nil {
				return nil, err
			}
			buf.Write(chunkData)

			crlf, _ := readLine(r)
			buf.WriteString(crlf + "\r\n")
		}
	} else if contentLength > 0 {
		body := make([]byte, contentLength)
		if _, err := io.ReadFull(r, body); err != nil {
			return nil, err
		}
		buf.Write(body)
	} else if contentLength == -1 {
		rest, _ := ioutil.ReadAll(r)
		buf.Write(rest)
	}

	return buf.Bytes(), nil
}

func errResponse(code int, msg string) []byte {
	body := fmt.Sprintf("%d %s\n", code, msg)
	return []byte(fmt.Sprintf(
		"HTTP %d %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
		code, msg, len(body), body,
	))
}

func readLine(r *bufio.Reader) (string, error) {
	line, err := r.ReadString('\n')
	line = strings.TrimRight(line, "\r\n")
	return line, err
}
