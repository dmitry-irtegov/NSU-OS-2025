package client

import (
	"fmt"
	"net"

	"golang.org/x/sys/unix"
)

func connectUpstream(request string) (Upst int, Err error) {
	reqKey, err := parseRequest(request)
	if err != nil {
		return 0, fmt.Errorf("cannot parse http-request: %w", err)
	}

	if reqKey.method != "GET" {
		return 0, fmt.Errorf("only GET method is supported")
	}

	ips, err := net.LookupIP(reqKey.host)
	if err != nil {
		return 0, fmt.Errorf("dns resolution failed for [%s]: %w", reqKey.host, err)
	}

	var ipv4 net.IP
	for _, ip := range ips {
		if ipv4 = ip.To4(); ipv4 != nil {
			break
		}
	}

	if ipv4 == nil {
		return 0, fmt.Errorf("no A record (IPv4) found for %s", reqKey.host)
	}

	addr := &unix.SockaddrInet4{Port: reqKey.port}
	copy(addr.Addr[:], ipv4)

	upstreamSocket, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return 0, fmt.Errorf("cannot create socket: %w", err)
	}

	if err := unix.Connect(upstreamSocket, addr); err != nil {
		return 0, fmt.Errorf("cannot connect to upstream: %w", err)
	}

	return upstreamSocket, nil
}
