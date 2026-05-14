package server

import (
	"fmt"

	"golang.org/x/sys/unix"
)

func (s *Server) TurnOn(limit int) error {
	if err := unix.Bind(s.socket, &unix.SockaddrInet4{
		Port: 8080,
		Addr: [4]byte{0, 0, 0, 0},
	}); err != nil {
		return fmt.Errorf("cannot bind the server's socket to the address: %w", err)
	}

	if err := unix.Listen(s.socket, limit); err != nil {
		return fmt.Errorf("cannot turn the server's socket to listen-mode: %w", err)
	}

	s.socketChannel = make(chan int, limit)

	return nil
}
