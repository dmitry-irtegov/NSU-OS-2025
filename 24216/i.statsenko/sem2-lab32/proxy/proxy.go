package proxy

import (
	"syscall"

	"golang.org/x/sys/unix"
)

type Proxy struct {
	listenFd int
	cache    *Cache
}

func NewProxy(port int) (*Proxy, error) {
	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
	if err != nil {
		return nil, err
	}
	if err := unix.SetsockoptInt(fd, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1); err != nil {
		unix.Close(fd)
		return nil, err
	}
	addr := &unix.SockaddrInet4{Port: port}
	if err := unix.Bind(fd, addr); err != nil {
		unix.Close(fd)
		return nil, err
	}
	if err := syscall.Listen(fd, 128); err != nil {
		unix.Close(fd)
		return nil, err
	}
	return &Proxy{listenFd: fd, cache: NewCache()}, nil
}

func (p *Proxy) Run() error {
	defer func() {
		unix.Shutdown(p.listenFd, unix.SHUT_RDWR)
		unix.Close(p.listenFd)
	}()

	for {
		fd, _, err := unix.Accept(p.listenFd)
		if err != nil {
			if err == unix.EINTR {
				continue
			}
			return err
		}
		go p.handleConn(fd)
	}
}
