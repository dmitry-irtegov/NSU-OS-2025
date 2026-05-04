package proxy

import (
	"syscall"

	"golang.org/x/sys/unix"
)

type Proxy struct {
	listenFd int
	workers  []*Worker
	next     int
}

func NewProxy(port, numWorkers int) (*Proxy, error) {
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

	cache := NewCache()
	workers := make([]*Worker, numWorkers)
	for i := range workers {
		w, err := newWorker(cache)
		if err != nil {
			unix.Shutdown(fd, unix.SHUT_RDWR)
			unix.Close(fd)
			return nil, err
		}
		workers[i] = w
	}

	return &Proxy{listenFd: fd, workers: workers}, nil
}

func (p *Proxy) Run() error {
	defer func() {
		unix.Shutdown(p.listenFd, unix.SHUT_RDWR)
		unix.Close(p.listenFd)
	}()

	for _, w := range p.workers {
		go w.run()
	}

	for {
		fd, _, err := unix.Accept(p.listenFd)
		if err != nil {
			if err == unix.EINTR {
				continue
			}
			return err
		}
		p.workers[p.next].assign(fd)
		p.next = (p.next + 1) % len(p.workers)
	}
}
