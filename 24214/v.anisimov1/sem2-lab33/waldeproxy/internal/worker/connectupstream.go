package worker

import (
	"fmt"
	"net"
	"waldeproxy/internal/request"

	"golang.org/x/sys/unix"
)

func (w *Worker) connectUpstream(requestMeta *request.RequestMeta) (Upst int, Err error) {
	if requestMeta.GetMethod() != "GET" {
		return 0, fmt.Errorf("only GET method is supported")
	}

	w.normalLogger.Printf("connecting to the host [%s]...\n", requestMeta.GetHost())

	ips, err := net.LookupIP(requestMeta.GetHost())
	if err != nil {
		return 0, fmt.Errorf("dns resolution failed for [%s]: %w", requestMeta.GetHost(), err)
	}

	var ipv4 net.IP
	for _, ip := range ips {
		if ipv4 = ip.To4(); ipv4 != nil {
			break
		}
	}

	if ipv4 == nil {
		return 0, fmt.Errorf("no A record (IPv4) found for %s", requestMeta.GetHost())
	}

	addr := &unix.SockaddrInet4{Port: requestMeta.GetPort()}
	copy(addr.Addr[:], ipv4)

	upstreamSocket, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return 0, fmt.Errorf("cannot create socket: %w", err)
	}

	if err := unix.SetNonblock(upstreamSocket, true); err != nil {
		unix.Close(upstreamSocket)
		return 0, fmt.Errorf("cannot set non-blocking socket: %w", err)
	}

	if err := unix.Connect(upstreamSocket, addr); err != nil {
		if err != unix.EINPROGRESS {
			unix.Close(upstreamSocket)
			return 0, fmt.Errorf("cannot connect to upstream: %w", err)
		} else {
			return upstreamSocket, nil
		}
	}

	return upstreamSocket, nil
}
