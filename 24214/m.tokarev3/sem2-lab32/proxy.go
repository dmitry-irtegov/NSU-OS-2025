package main

import (
	"bufio"
	"log"
	"net"
	"strings"
	"sync/atomic"
)

type Proxy struct {
	addr   string
	cache  *Cache
	quit   chan struct{}
	nextID uint64
}

func NewProxy(addr string, cache *Cache) *Proxy {
	return &Proxy{addr: addr, cache: cache, quit: make(chan struct{})}
}

func (p *Proxy) Stop() {
	close(p.quit)
}

func (p *Proxy) Run() error {
	ln, err := net.Listen("tcp", p.addr)
	if err != nil {
		return err
	}
	defer ln.Close()

	go func() {
		<-p.quit
		ln.Close()
	}()

	for {
		conn, err := ln.Accept()
		if err != nil {
			select {
			case <-p.quit:
				return nil
			default:
				log.Printf("accept error: %v", err)
				continue
			}
		}

		connID := atomic.AddUint64(&p.nextID, 1)
		log.Printf("new connection from %s (id=%d)", conn.RemoteAddr(), connID)
		go p.handleClient(conn, connID)
	}
}

func (p *Proxy) handleClient(conn net.Conn, connID uint64) {
	defer conn.Close()

	reader := bufio.NewReader(conn)
	req, err := parseRequest(reader)
	if err != nil {
		if !isTemporary(err) {
			log.Printf("id=%d parse error: %v", connID, err)
		}
		return
	}

	log.Printf("id=%d %s %s", connID, req.Method, req.Target)

	if req.Method != "GET" && req.Method != "HEAD" {
		data := errResponse(405, "Method Not Allowed")
		conn.Write(data)
		log.Printf("id=%d response sent (%d bytes)", connID, len(data))
		return
	}

	if entry, hit := p.cache.Get(req.Target); hit {
		log.Printf("id=%d cache HIT for %s", connID, req.Target)
		conn.Write(entry.Data)
		log.Printf("id=%d response sent (%d bytes)", connID, len(entry.Data))
		return
	}

	log.Printf("id=%d cache MISS – connecting upstream for %s", connID, req.Target)

	upstreamData, err := fetchFromUpstream(req)
	if err != nil {
		log.Printf("id=%d upstream error: %v", connID, err)
		data := errResponse(502, "Bad Gateway")
		conn.Write(data)
		log.Printf("id=%d response sent (%d bytes)", connID, len(data))
		return
	}

	p.cache.Set(req.Target, upstreamData)
	log.Printf("id=%d cached %d bytes for %s", connID, len(upstreamData), req.Target)
	conn.Write(upstreamData)
	log.Printf("id=%d response sent (%d bytes)", connID, len(upstreamData))
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
