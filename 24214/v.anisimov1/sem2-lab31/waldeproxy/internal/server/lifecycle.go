package server

import (
	"fmt"
	"log"
	"os"

	"golang.org/x/sys/unix"
)

func createServer(limit int) (*server, error) {
	socketfd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)

	if err != nil {
		return nil, fmt.Errorf("could not create a tcp-socket: %w", err)
	}

	err = unix.SetNonblock(socketfd, true)

	if err != nil {
		unix.Close(socketfd)
		return nil, fmt.Errorf("cannot make the server unblock: %w", err)
	}

	err = unix.SetsockoptInt(socketfd, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1)

	if err != nil {
		unix.Close(socketfd)
		return nil, fmt.Errorf("cannot set server option: %w", err)
	}

	return &server{
			activeClients: 0,
			socket:        socketfd,
			limit:         limit,
			pollfdset:     make([]unix.PollFd, 0, limit+1),
			sessions:      make(map[int]session),
			respCache:     initCache(),
			errLogger:     log.New(os.Stderr, "SERVER: ERROR! >>> ", 0),
			normalLogger:  log.New(os.Stdout, "SERVER: REPORT ---> ", 0),
		},
		nil
}

func (s *server) turnon() error {
	addr := &unix.SockaddrInet4{Port: port, Addr: serverIPAddr}

	err := unix.Bind(s.socket, addr)

	if err != nil {
		return fmt.Errorf("cannot bind the socket to the address: %w", err)
	}

	err = unix.Listen(s.socket, s.limit)

	if err != nil {
		return fmt.Errorf("cannot start listening: %w", err)
	}

	s.pollfdset = append(
		s.pollfdset,
		unix.PollFd{Fd: int32(s.socket), Events: unix.POLLIN, Revents: 0},
	)

	s.normalLogger.Println("proxy has started listening on port 8080")

	return nil
}

func (s *server) iteration() error {
	n, err := unix.Poll(s.pollfdset, -1)

	if err == unix.EINTR {
		return nil
	}

	if err != nil {
		return fmt.Errorf("[poll] system-call cannot handle events: %w", err)
	}

	for pollfdIdx := 0; n > 0 && pollfdIdx < len(s.pollfdset); pollfdIdx++ {
		if s.pollfdset[pollfdIdx].Revents == 0 {
			continue
		}

		n--

		if s.pollfdset[pollfdIdx].Revents&(unix.POLLHUP|unix.POLLERR|unix.POLLNVAL) != 0 {
			s.closeBadSession(pollfdIdx)
			continue
		}

		if pollfdIdx == 0 && s.pollfdset[pollfdIdx].Revents&unix.POLLIN != 0 {
			if s.activeClients >= s.limit {
				continue
			}

			s.pollfdset[pollfdIdx].Revents = 0
			client, err := s.acceptClient()
			if err != nil {
				s.errLogger.Printf("cannot accept new clients: %v\n", err)
			}

			if client == 0 {
				continue
			}

			s.pollfdset = append(
				s.pollfdset,
				unix.PollFd{Fd: int32(client), Events: unix.POLLIN, Revents: 0},
			)

			s.sessions[client] = &clientSession{
				socketfd:   client,
				state:      readClientRequest,
				buffer:     make([]byte, 1024),
				bufferSize: 0,
				wroteBytes: 0,
			}

			s.activeClients++
			continue
		}

		session := s.sessions[int(s.pollfdset[pollfdIdx].Fd)]

		if err := s.handleSession(session, pollfdIdx); err != nil {
			if err != errSessionComplete {
				s.errLogger.Printf("not successful handling: %v", err)
			}
			s.closeBadSession(pollfdIdx)
		}

		s.pollfdset[pollfdIdx].Revents = 0
	}

	s.clearDisconnectedSessions()

	return nil
}

func (s *server) destroy() {
	for i := 1; i < len(s.pollfdset); i++ {
		unix.Close(int(s.pollfdset[i].Fd))
	}
	unix.Close(s.socket)
}
