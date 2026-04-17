package main

import (
	"log"
	"os"
	"sem2-lab28/client"
)

func main() {
	if len(os.Args) != 2 {
		log.Fatal("Usage: go run main.go <url>")
	}
	cl, err := client.NewClient(os.Args[1])
	if err != nil {
		log.Fatal(err)
	}
	if err := cl.Run(); err != nil {
		log.Fatal(err)
	}
}
