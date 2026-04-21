package server

import (
	"bytes"
	"fmt"
	"net"
	"strconv"
	"strings"

	"golang.org/x/sys/unix"
)

func (s *server) handleReadClientRequest(session *clientSession, pollfdsetIdx int) error {
	revents := s.pollfdset[pollfdsetIdx].Revents

	if revents&unix.POLLIN != 0 {
		if session.bufferSize == len(session.buffer) {
			newBuf := make([]byte, len(session.buffer)*2)
			copy(newBuf, session.buffer)
			session.buffer = newBuf
		}

		n, err := unix.Read(
			session.socketfd,
			session.buffer[session.bufferSize:],
		)

		if err == unix.EAGAIN {
			return nil
		}

		if err != nil {
			return fmt.Errorf("cannot get data from a socket: %w", err)
		}

		if n == 0 {
			return fmt.Errorf("client closed connection")
		}

		session.bufferSize += n

		if headerEndIdx := bytes.Index(session.buffer[:session.bufferSize], []byte("\r\n\r\n")); headerEndIdx != -1 {
			normalized := prepareRequest(session.buffer[:session.bufferSize])
			requestParams, err := parseRequest(string(normalized))
			if err != nil {
				return fmt.Errorf("cannot parse http-request")
			}

			key := &cacheKey{
				host:     requestParams.host,
				resource: requestParams.resource,
			}

			entry, err := s.respCache.get(key)
			if err != nil {
				s.pollfdset[pollfdsetIdx].Events = 0

				upstrSock, _, err := s.connectUpstream(string(normalized))
				if err != nil {
					return fmt.Errorf("cannot connect to the upstream %v", err)
				}

				s.pollfdset = append(s.pollfdset, unix.PollFd{
					Fd:      int32(upstrSock),
					Events:  unix.POLLOUT,
					Revents: 0,
				})

				entry := &cacheEntry{
					isReady: false,
					waiters: []*clientSession{session},
				}

				s.respCache.entries[*(key)] = entry

				upstrSess := &upstreamSession{
					socketfd:   upstrSock,
					state:      sendRequestToUpstream,
					buffer:     make([]byte, 0, 1024),
					wroteBytes: 0,
					key:        key,
					entry:      entry,
				}
				upstrSess.buffer = append(upstrSess.buffer, normalized...)
				upstrSess.bufferSize = len(normalized)
				s.sessions[upstrSock] = upstrSess

				s.normalLogger.Println("request was read successfully --> waiting for response...")
				session.state = sendResponseToClient
				return nil
			}

			if data, err := entry.getData(); err != nil {
				s.pollfdset[pollfdsetIdx].Events = 0
				entry.waiters = append(entry.waiters, session)
				s.normalLogger.Println("request was read successfully --> waiting for response...")

				session.state = sendResponseToClient
				return nil
			} else {
				session.buffer = data
				session.bufferSize = len(data)
				s.pollfdset[pollfdsetIdx].Events = unix.POLLOUT
			}

			session.state = sendResponseToClient
			s.normalLogger.Println("response extracted from cache")
		}
	}

	return nil
}

func (s *server) handleSendResponseToClient(session *clientSession, pollfdsetIdx int) error {
	revents := s.pollfdset[pollfdsetIdx].Revents

	if revents&unix.POLLOUT != 0 {
		n, err := unix.Write(
			session.socketfd,
			session.buffer[session.wroteBytes:],
		)

		if err == unix.EAGAIN {
			return nil
		}

		if err != nil {
			return fmt.Errorf("cannot write to upstream's socket: %w", err)
		}

		session.wroteBytes += n

		if session.wroteBytes == session.bufferSize {
			s.normalLogger.Println("response was sent successfully")
			return errSessionComplete
		}
	}

	return nil
}

func (s *server) handleSendRequestToUpstream(session *upstreamSession, pollfdsetIdx int) error {
	revents := s.pollfdset[pollfdsetIdx].Revents

	if revents&unix.POLLOUT != 0 {
		n, err := unix.Write(
			session.socketfd,
			session.buffer[session.wroteBytes:session.bufferSize],
		)

		if err == unix.EAGAIN {
			return nil
		}

		if err != nil {
			return fmt.Errorf("cannot write to client's socket: %w", err)
		}

		session.wroteBytes += n

		if session.wroteBytes == session.bufferSize {
			session.state = readUpstremResponse
			session.wroteBytes = 0
			session.buffer = make([]byte, 1024)
			session.bufferSize = 0
			s.pollfdset[pollfdsetIdx].Events = unix.POLLIN
			s.normalLogger.Println("request was sent to upstream successfully")
		}
	}

	return nil
}

func (s *server) handleReadUpstreamResponse(session *upstreamSession, pollfdsetIdx int) error {
	revents := s.pollfdset[pollfdsetIdx].Revents

	if revents&unix.POLLIN != 0 {
		if session.bufferSize == len(session.buffer) {
			newBuf := make([]byte, len(session.buffer)*2)
			copy(newBuf, session.buffer)
			session.buffer = newBuf
		}

		n, err := unix.Read(
			session.socketfd,
			session.buffer[session.bufferSize:],
		)

		if err == unix.EAGAIN {
			return nil
		}

		if err != nil {
			return fmt.Errorf("cannot get data from a socket: %w", err)
		}

		if n == 0 {
			return fmt.Errorf("upstream closed connection")
		}

		session.bufferSize += n

		headerEndIdx := bytes.Index(session.buffer[:session.bufferSize], []byte("\r\n\r\n"))

		if headerEndIdx == -1 {
			return nil
		}

		bodySize := getExpectedBodySize(session.buffer[:headerEndIdx])

		expectedTotalSize := headerEndIdx + 4 + bodySize

		if session.bufferSize >= expectedTotalSize {
			bufferCopy := make([]byte, session.bufferSize)

			copy(bufferCopy, session.buffer[:session.bufferSize])

			for _, client := range session.entry.waiters {
				if _, ok := s.sessions[client.socketfd]; !ok {
					continue
				}

				client.buffer = bufferCopy
				client.bufferSize = len(bufferCopy)
				s.pollfdset[s.findPollIdx(client.socketfd)].Events = unix.POLLOUT
				client.state = sendResponseToClient
			}

			s.respCache.putResponse(session.key, bufferCopy)
			return errSessionComplete
		}
	}

	return nil
}

func (s *server) connectUpstream(request string) (Upst int, InProg bool, Err error) {
	reqKey, err := parseRequest(request)
	if err != nil {
		return 0, false, fmt.Errorf("cannot parse http-request: %w", err)
	}

	if reqKey.method != "GET" {
		return 0, false, fmt.Errorf("only GET method is supported")
	}

	s.normalLogger.Printf("connecting to the host [%s]...\n", reqKey.host)

	ips, err := net.LookupIP(reqKey.host)
	if err != nil {
		return 0, false, fmt.Errorf("dns resolution failed for [%s]: %w", reqKey.host, err)
	}

	var ipv4 net.IP
	for _, ip := range ips {
		if ipv4 = ip.To4(); ipv4 != nil {
			break
		}
	}

	if ipv4 == nil {
		return 0, false, fmt.Errorf("no A record (IPv4) found for %s", reqKey.host)
	}

	addr := &unix.SockaddrInet4{Port: reqKey.port}
	copy(addr.Addr[:], ipv4)

	upstreamSocket, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return 0, false, fmt.Errorf("cannot create socket: %w", err)
	}

	if err := unix.SetNonblock(upstreamSocket, true); err != nil {
		unix.Close(upstreamSocket)
		return 0, false, fmt.Errorf("cannot set non-blocking socket: %w", err)
	}

	if err := unix.Connect(upstreamSocket, addr); err != nil {
		if err != unix.EINPROGRESS {
			unix.Close(upstreamSocket)
			return 0, false, fmt.Errorf("cannot connect to upstream: %w", err)
		} else {
			s.normalLogger.Println("connecting new upstream...")
			return upstreamSocket, true, nil
		}
	}

	s.normalLogger.Println("connected to upstream succesfully")
	return upstreamSocket, false, nil
}

type requestParams struct {
	method   string
	resource string
	host     string
	port     int
}

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

func getExpectedBodySize(headers []byte) int {
	headerStr := string(headers)

	lines := strings.Split(headerStr, "\r\n")

	for _, line := range lines {
		lowerLine := strings.ToLower(line)

		if strings.HasPrefix(lowerLine, "content-length:") {
			parts := strings.SplitN(line, ":", 2)
			if len(parts) == 2 {
				valStr := strings.TrimSpace(parts[1])

				size, err := strconv.Atoi(valStr)
				if err == nil {
					return size
				}
			}
		}
	}

	return 0
}
