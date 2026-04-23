package proxy

import (
	"fmt"
	"net"
	"strconv"
	"strings"
)

func (p *Proxy) handleConn(conn *net.TCPConn) {
	defer conn.Close()

	reqBuf, err := p.readUntilDoubleCRLF(conn)
	if err != nil {
		return
	}

	host, port, path, cacheKey, err := p.parseRequest(string(reqBuf))
	if err != nil {
		return
	}

	if data, ok := p.cache.Get(cacheKey); ok {
		p.writeAll(conn, data)
		return
	}

	upstream, err := net.Dial("tcp4", fmt.Sprintf("%s:%d", host, port))
	if err != nil {
		return
	}
	defer upstream.Close()

	req := fmt.Sprintf("GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host)
	if err := p.writeAll(upstream, []byte(req)); err != nil {
		return
	}

	var respBuf []byte
	var tmp [4096]byte
	for {
		n, err := upstream.Read(tmp[:])
		if n > 0 {
			chunk := tmp[:n]
			respBuf = append(respBuf, chunk...)
			if werr := p.writeAll(conn, chunk); werr != nil {
				return
			}
		}
		if err != nil || n == 0 {
			break
		}
	}

	p.cache.Set(cacheKey, respBuf)
}

func (p *Proxy) readUntilDoubleCRLF(conn net.Conn) ([]byte, error) {
	var buf []byte
	tmp := make([]byte, 4096)
	for {
		n, err := conn.Read(tmp)
		if n > 0 {
			buf = append(buf, tmp[:n]...)
			if p.findDoubleCRLF(buf) >= 0 {
				return buf, nil
			}
		}
		if err != nil || n == 0 {
			return nil, fmt.Errorf("connection closed before complete request")
		}
	}
}

func (p *Proxy) writeAll(conn net.Conn, data []byte) error {
	for len(data) > 0 {
		n, err := conn.Write(data)
		if err != nil {
			return err
		}
		data = data[n:]
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

func (p *Proxy) findDoubleCRLF(data []byte) int {
	for i := 0; i+3 < len(data); i++ {
		if data[i] == '\r' && data[i+1] == '\n' && data[i+2] == '\r' && data[i+3] == '\n' {
			return i
		}
	}
	return -1
}
