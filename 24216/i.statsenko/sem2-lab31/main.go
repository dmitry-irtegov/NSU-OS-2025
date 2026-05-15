package main

import (
	"log"
	"os"
	"strconv"

	"sem2-lab31/proxy"
)

func main() {
	if len(os.Args) != 2 {
		log.Fatal("Usage: go run main.go <port>")
	}
	port, err := strconv.Atoi(os.Args[1])
	if err != nil {
		log.Fatal("invalid port:", err)
	}
	p, err := proxy.NewProxy(port)
	if err != nil {
		log.Fatal(err)
	}
	if err := p.Run(); err != nil {
		log.Fatal(err)
	}
}
