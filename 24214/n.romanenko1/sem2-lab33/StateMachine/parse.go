package statemachine

import (
	"fmt"
	"net"
	"strings"
)

func resolveHostPort(host string, defaultPort int) ([4]byte, int) {
	hostParts := strings.Split(host, ":")
	hostname := hostParts[0]

	port := defaultPort
	if len(hostParts) > 1 {
		fmt.Sscanf(hostParts[1], "%d", &port)
	}

	ips, err := net.LookupIP(hostname)
	if err != nil || len(ips) == 0 {
		return [4]byte{127, 0, 0, 1}, port
	}

	ipv4 := ips[0].To4()
	if ipv4 == nil {
		return [4]byte{127, 0, 0, 1}, port
	}

	return [4]byte{ipv4[0], ipv4[1], ipv4[2], ipv4[3]}, port
}

func parseRequest(data []byte) (url, host string, ok bool) {
	lines := strings.Split(string(data), "\r\n")
	if len(lines) == 0 {
		return
	}

	parts := strings.Split(lines[0], " ")
	if len(parts) < 3 {
		return
	}

	url = parts[1]

	for _, line := range lines[1:] {
		if strings.HasPrefix(strings.ToLower(line), "host: ") {
			host = strings.TrimSpace(line[5:])
			break
		}
	}

	ok = true
	return
}

func parseContentLength(headers string) int {
	lines := strings.SplitSeq(headers, "\r\n")
	for line := range lines {
		lowerLine := strings.ToLower(line)
		if strings.HasPrefix(lowerLine, "content-length:") {
			var length int
			fmt.Sscanf(lowerLine, "content-length: %d", &length)
			return length
		}
	}
	return 0
}
