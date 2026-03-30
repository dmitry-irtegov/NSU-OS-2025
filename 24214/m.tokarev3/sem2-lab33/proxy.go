package main

import (
	"bufio"
	"log"
	"net"
	"strings"
)

type connState int

const (
	stateReadRequest connState = iota
	stateConnUpstream
	stateWriteUpstream
	stateReadUpstream
	stateWriteClient
)

type clientConn struct {
	conn   net.Conn
	fd     int
	state  connState
	reader *bufio.Reader
	req    *Request

	upFd       int
	upReqBytes []byte
	upSent     int
	upBuf      []byte

	response []byte
	sent     int
}

type Proxy struct {
	addr     string
	cache    *Cache
	poolSize int
	listener net.Listener
	quit     chan struct{}
	workers  []*Worker
}

func NewProxy(addr string, cache *Cache, poolSize int) *Proxy {
	return &Proxy{
		addr:     addr,
		cache:    cache,
		poolSize: poolSize,
		quit:     make(chan struct{}),
	}
}

func (p *Proxy) Stop() {
	close(p.quit)
	if p.listener != nil {
		p.listener.Close()
	}
}

func (p *Proxy) Run() error {
	ln, err := net.Listen("tcp", p.addr)
	if err != nil {
		return err
	}
	p.listener = ln
	defer p.listener.Close()

	p.workers = make([]*Worker, p.poolSize)
	for i := 0; i < p.poolSize; i++ {
		w, err := NewWorker(i, p.cache)
		if err != nil {
			return err
		}
		p.workers[i] = w
		go w.Run(p.quit)
	}

	log.Printf("started %d worker threads", p.poolSize)

	next := 0
	for {
		conn, err := p.listener.Accept()
		if err != nil {
			select {
			case <-p.quit:
				return nil
			default:
				log.Printf("accept error: %v", err)
				continue
			}
		}

		p.workers[next].AddConn(conn)
		next = (next + 1) % p.poolSize
	}
}

func connFd(conn *net.TCPConn) (int, error) {
	raw, err := conn.SyscallConn()
	if err != nil {
		return 0, err
	}
	var fd int
	raw.Control(func(f uintptr) { fd = int(f) })
	return fd, nil
}

func isTemporary(err error) bool {
	if err == nil {
		return false
	}
	s := err.Error()
	return strings.Contains(s, "resource temporarily unavailable") ||
		strings.Contains(s, "try again") ||
		strings.Contains(s, "would block")
}
