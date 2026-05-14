package server

import (
	"fmt"
	"log"
	"os"
	"waldeproxy/internal/cache"

	"golang.org/x/sys/unix"
)

func NewServer(workerAmount int) (*Server, error) {
	socketfd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)

	if err != nil {
		return nil, fmt.Errorf("could not create a tcp-socket: %w", err)
	}

	if err := unix.SetsockoptInt(socketfd, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1); err != nil {
		unix.Close(socketfd)
		return nil, fmt.Errorf("cannot set server option: %w", err)
	}

	return &Server{
			socket:       socketfd,
			workerAmount: workerAmount,
			respCache:    cache.InitCache(),
			errLogger:    log.New(os.Stderr, "server: error  >>> ", 0),
			normalLogger: log.New(os.Stdout, "server: report >>> ", 0),
		},
		nil
}
