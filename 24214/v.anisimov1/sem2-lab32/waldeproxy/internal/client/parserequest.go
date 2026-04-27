package client

import (
	"fmt"
	"net"
	"strconv"
	"strings"
)

func parseRequest(request string) (*requestParams, error) {
	lines := strings.Split(request, "\r\n")
	if len(lines) < 2 {
		return nil, fmt.Errorf("invalid http request: missing CRLF")
	}

	tokens := strings.Split(lines[0], " ")
	if len(tokens) != 3 {
		return nil, fmt.Errorf("invalid http request: malformed start line")
	}

	method := tokens[0]
	resource := tokens[1]
	var rawHost string

	for i := 1; i < len(lines); i++ {
		line := lines[i]
		if line == "" {
			break
		}
		if strings.HasPrefix(strings.ToLower(line), "host:") {
			rawHost = strings.TrimSpace(line[5:])
			break
		}
	}

	if rawHost == "" {
		return nil, fmt.Errorf("invalid http request: missing Host header")
	}

	host := rawHost
	port := 80

	if h, pStr, err := net.SplitHostPort(rawHost); err == nil {
		host = h
		if p, err := strconv.Atoi(pStr); err == nil {
			port = p
		}
	}

	return &requestParams{method: method, resource: resource, host: host, port: port}, nil
}
