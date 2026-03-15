package main

import (
	"flag"
	"log"
	"os"
	"os/signal"
	"syscall"
)

func main() {
	addr := flag.String("addr", ":8080", "proxy listen address")
	flag.Parse()

	cache := NewCache()
	proxy := NewProxy(*addr, cache)

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-sigCh
		log.Println("shutting down…")
		proxy.Stop()
	}()

	log.Printf("caching proxy listening on %s (single-thread / poll loop)", *addr)
	if err := proxy.Run(); err != nil {
		log.Fatal(err)
	}
}
