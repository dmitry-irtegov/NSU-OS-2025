package server

import (
	"fmt"
	"waldeproxy/internal/client"

	"golang.org/x/sys/unix"
)

func (s *Server) AcceptClient(id int) (*client.ClientContext, error) {
	clientfd, _, err := unix.Accept(s.socketfd)

	if err != nil {
		return nil, fmt.Errorf("cannot accept client: %w", err)
	}

	return client.InitClientContext(id, clientfd, s.respCache, s.errLogger, s.normalLogger), nil
}
