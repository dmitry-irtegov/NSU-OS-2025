package main

import (
	"laba/client"
	"laba/parser"
	"log"
	"os"
)

func main() {
	if len(os.Args) != 2 {
		log.Fatal("Usage: go run main.go <address>")
	}
	ps := parser.NewParser()
	cl, err := client.NewClient(ps, os.Args[1])
	if err != nil {
		log.Fatal(err)
	}
	if err := cl.Run(); err != nil {
		log.Fatal(err)
	}
}
