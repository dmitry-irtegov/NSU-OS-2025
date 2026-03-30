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
	poolSize := flag.Int("pool", 4, "number of worker threads")
	flag.Parse()

	if *poolSize < 1 {
		log.Fatal("pool size must be at least 1")
	}

	cache := NewCache()
	proxy := NewProxy(*addr, cache, *poolSize)

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-sigCh
		log.Println("shutting down…")
		proxy.Stop()
	}()

	log.Printf("caching proxy listening on %s (worker pool, %d threads)", *addr, *poolSize)
	if err := proxy.Run(); err != nil {
		log.Fatal(err)
	}
}
