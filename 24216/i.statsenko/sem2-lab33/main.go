package main

import (
	"log"
	"os"
	"strconv"

	"sem2-lab33/proxy"
)

func main() {
	if len(os.Args) != 3 {
		log.Fatal("Usage: go run main.go <port> <workers>")
	}
	port, err := strconv.Atoi(os.Args[1])
	if err != nil {
		log.Fatal("invalid port:", err)
	}
	numWorkers, err := strconv.Atoi(os.Args[2])
	if err != nil || numWorkers < 1 {
		log.Fatal("invalid workers count:", err)
	}
	p, err := proxy.NewProxy(port, numWorkers)
	if err != nil {
		log.Fatal(err)
	}
	if err := p.Run(); err != nil {
		log.Fatal(err)
	}
}
