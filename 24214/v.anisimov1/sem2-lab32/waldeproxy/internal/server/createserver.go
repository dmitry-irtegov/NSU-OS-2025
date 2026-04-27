package server

import (
	"fmt"
	"waldeproxy/internal/cache"
	"waldeproxy/internal/threadsafelogger"

	"golang.org/x/sys/unix"
)

func InitServer(
	limit int,
	errLogger *threadsafelogger.ErrLogger,
	normalLogger *threadsafelogger.NormalLogger) (*Server, error) {
	socket, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)

	if err != nil {
		return nil, fmt.Errorf("failed to create socket: %v", err)
	}

	addr := &unix.SockaddrInet4{Port: 8080, Addr: [4]byte{0, 0, 0, 0}}

	if err := unix.Bind(socket, addr); err != nil {
		unix.Close(socket)
		return nil, fmt.Errorf("failed to bind socket: %v", err)
	}

	if err := unix.SetsockoptInt(socket, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1); err != nil {
		unix.Close(socket)
		return nil, fmt.Errorf("failed to set opetion: %w", err)
	}

	if err := unix.Listen(socket, limit); err != nil {
		unix.Close(socket)
		return nil, fmt.Errorf("failed to turn up the server: %w", err)
	}

	return &Server{
		socketfd:     socket,
		respCache:    cache.InitCache(),
		errLogger:    errLogger,
		normalLogger: normalLogger,
	}, nil
}
