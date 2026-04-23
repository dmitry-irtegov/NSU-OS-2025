package proxy

import (
	"fmt"
	"net"
)

type Proxy struct {
	ln    *net.TCPListener
	cache *Cache
}

func NewProxy(port int) (*Proxy, error) {
	ln, err := net.Listen("tcp4", fmt.Sprintf(":%d", port))
	if err != nil {
		return nil, err
	}
	return &Proxy{
		ln:    ln.(*net.TCPListener),
		cache: NewCache(),
	}, nil
}

func (p *Proxy) Run() error {
	defer p.ln.Close()
	for {
		conn, err := p.ln.AcceptTCP()
		if err != nil {
			return err
		}
		go p.handleConn(conn)
	}
}
