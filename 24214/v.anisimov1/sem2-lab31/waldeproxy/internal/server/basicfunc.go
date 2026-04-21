package server

import (
	"fmt"
	"net/url"
	"strings"

	"golang.org/x/sys/unix"
)

func (s *server) acceptClient() (int, error) {
	client, _, err := unix.Accept(s.socket)

	if err == unix.EAGAIN || err == unix.EWOULDBLOCK {
		return 0, nil
	}

	if err != nil {
		return 0, fmt.Errorf("cannot accept client: %w", err)
	}

	if err := unix.SetNonblock(client, true); err != nil {
		return 0, fmt.Errorf("cannot set non-blocking client: %w", err)
	}

	return client, nil
}

func (s *server) handleSession(session session, pollfdsetIdx int) error {
	if session.getType() == typeClient {
		clientSess := session.(*clientSession)

		switch clientSess.state {
		case readClientRequest:

			if err := s.handleReadClientRequest(clientSess, pollfdsetIdx); err != nil {
				return fmt.Errorf("cannot handle client-session: %w", err)
			}

		case sendResponseToClient:

			err := s.handleSendResponseToClient(clientSess, pollfdsetIdx)
			if err != nil && err != errSessionComplete {
				return fmt.Errorf("cannot handle client-session: %w", err)
			}
			return err

		}
	} else {
		upstrSession := session.(*upstreamSession)
		switch upstrSession.state {
		case sendRequestToUpstream:

			if err := s.handleSendRequestToUpstream(upstrSession, pollfdsetIdx); err != nil {
				return fmt.Errorf("cannot handle upstream-session: %w", err)
			}

		case readUpstremResponse:

			err := s.handleReadUpstreamResponse(upstrSession, pollfdsetIdx)
			if err != nil && err != errSessionComplete {
				return fmt.Errorf("cannot handle upstream-session: %w", err)
			}
			return err
		}
	}

	return nil
}

func (s *server) closeBadSession(pollfdIdx int) {
	if session, ok := s.sessions[int(s.pollfdset[pollfdIdx].Fd)]; ok {
		if session.getType() == typeUpstream {
			upstream := session.(*upstreamSession)
			if entry, ok := s.respCache.entries[*upstream.key]; ok {
				if !entry.isReady {
					for _, client := range entry.waiters {
						clientPollIdx := s.findPollIdx(client.socketfd)
						if clientPollIdx != -1 {
							delete(s.sessions, client.socketfd)
							unix.Close(client.socketfd)
							s.pollfdset[clientPollIdx].Fd = -1
							s.activeClients--
						}
					}
					delete(s.respCache.entries, *upstream.key)
				}
			}
		} else {
			s.activeClients--
		}
	}
	delete(s.sessions, int(s.pollfdset[pollfdIdx].Fd))
	unix.Close(int(s.pollfdset[pollfdIdx].Fd))
	s.pollfdset[pollfdIdx].Fd = -1
}

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
		if strings.HasPrefix(low, "connection:") ||
			strings.HasPrefix(low, "keep-alive:") ||
			strings.HasPrefix(low, "transfer-encoding:") ||
			strings.HasPrefix(low, "proxy-connection:") {
			continue
		}
		filtered = append(filtered, line)
	}

	filtered = append(filtered, "Connection: close")

	return []byte(startline + "\r\n" + strings.Join(filtered, "\r\n") + "\r\n\r\n")
}

func (s *server) clearDisconnectedSessions() {
	active := s.pollfdset[:0]
	for _, pfd := range s.pollfdset {
		if pfd.Fd != -1 {
			active = append(active, pfd)
		}
	}
	s.pollfdset = active
}

func (s *server) findPollIdx(fd int) int {
	for i, pfd := range s.pollfdset {
		if int(pfd.Fd) == fd {
			return i
		}
	}
	return -1
}
