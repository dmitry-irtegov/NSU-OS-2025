package client

import (
	"net/url"
	"strings"
)

func prepareRequest(req []byte) []byte {
	s := string(req)

	lines := strings.Split(s, "\r\n")

	startline := lines[0]
	headers := lines[1:]

	startline = strings.Replace(startline, " HTTP/1.1", " HTTP/1.0", 1)

	tokens := strings.SplitN(startline, " ", 3)
	if len(tokens) == 3 {
		target := tokens[1]
		if strings.HasPrefix(target, "http://") || strings.HasPrefix(target, "https://") {
			if u, err := url.Parse(target); err == nil {
				tokens[1] = u.RequestURI()
			}
		}
		startline = strings.Join(tokens, " ")
	}

	filtered := make([]string, 0, len(lines))

	for _, line := range headers {
		if line == "" {
			continue
		}

		low := strings.ToLower(line)
		if strings.HasPrefix(low, "connection:") {
			continue
		}
		filtered = append(filtered, line)
	}

	filtered = append(filtered, "Connection: close")

	return []byte(startline + "\r\n" + strings.Join(filtered, "\r\n") + "\r\n\r\n")
}
