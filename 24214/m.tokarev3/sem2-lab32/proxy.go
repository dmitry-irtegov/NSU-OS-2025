package main

import (
	"bufio"
	"io/ioutil"
	"log"
	"net"
	"sync"
)

type Proxy struct {
	addr     string
	cache    *Cache
	listener net.Listener
	quit     chan struct{}
	wg       sync.WaitGroup
}

func NewProxy(addr string, cache *Cache) *Proxy {
	return &Proxy{addr: addr, cache: cache, quit: make(chan struct{})}
}

func (p *Proxy) Stop() {
	close(p.quit)
	if p.listener != nil {
		p.listener.Close()
	}
	p.wg.Wait()
}

func (p *Proxy) Run() error {
	ln, err := net.Listen("tcp", p.addr)
	if err != nil {
		return err
	}
	p.listener = ln
	defer p.listener.Close()

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

		p.wg.Add(1)
		go p.handleConnection(conn)
	}
}

func (p *Proxy) handleConnection(clientConn net.Conn) {
	defer p.wg.Done()
	defer clientConn.Close()

	log.Printf("new connection from %s", clientConn.RemoteAddr())

	reader := bufio.NewReader(clientConn)
	req, err := parseRequest(reader)
	if err != nil {
		log.Printf("parse error from %s: %v", clientConn.RemoteAddr(), err)
		return
	}

	log.Printf("%s %s %s", clientConn.RemoteAddr(), req.Method, req.Target)

	if req.Method != "GET" && req.Method != "HEAD" {
		clientConn.Write(errResponse(405, "Method Not Allowed"))
		return
	}

	if entry, hit := p.cache.Get(req.Target); hit {
		log.Printf("cache HIT for %s", req.Target)
		clientConn.Write(entry.Data)
		return
	}

	log.Printf("cache MISS – connecting upstream for %s", req.Target)

	addr, err := upstreamAddr(req)
	if err != nil {
		log.Printf("failed to compute upstream addr: %v", err)
		clientConn.Write(errResponse(502, "Bad Gateway"))
		return
	}

	upConn, err := net.Dial("tcp", addr)
	if err != nil {
		log.Printf("failed to connect upstream: %v", err)
		clientConn.Write(errResponse(502, "Bad Gateway"))
		return
	}
	defer upConn.Close()

	upReqBytes := buildUpstreamRequest(req)
	if _, err := upConn.Write(upReqBytes); err != nil {
		log.Printf("failed to write to upstream: %v", err)
		clientConn.Write(errResponse(502, "Bad Gateway"))
		return
	}

	upData, err := ioutil.ReadAll(upConn)
	if err != nil {
		log.Printf("failed to read from upstream: %v", err)
		clientConn.Write(errResponse(502, "Bad Gateway"))
		return
	}

	if len(upData) == 0 {
		clientConn.Write(errResponse(502, "Bad Gateway"))
		return
	}

	p.cache.Set(req.Target, upData)
	log.Printf("cached %d bytes for %s", len(upData), req.Target)

	clientConn.Write(upData)
}
