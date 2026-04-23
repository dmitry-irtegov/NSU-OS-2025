package proxy

import (
	"fmt"
	"net"
)

type Proxy struct {
	ln      *net.TCPListener
	workers []*Worker
	next    int
}

func NewProxy(port, numWorkers int) (*Proxy, error) {
	ln, err := net.Listen("tcp4", fmt.Sprintf(":%d", port))
	if err != nil {
		return nil, err
	}
	tcpLn := ln.(*net.TCPListener)

	cache := NewCache()
	workers := make([]*Worker, numWorkers)
	for i := range workers {
		w, err := newWorker(cache)
		if err != nil {
			tcpLn.Close()
			return nil, err
		}
		workers[i] = w
	}

	return &Proxy{ln: tcpLn, workers: workers}, nil
}

func (p *Proxy) Run() error {
	defer p.ln.Close()

	for _, w := range p.workers {
		go w.run()
	}

	for {
		conn, err := p.ln.AcceptTCP()
		if err != nil {
			return err
		}
		p.workers[p.next].assign(conn)
		p.next = (p.next + 1) % len(p.workers)
	}
}
