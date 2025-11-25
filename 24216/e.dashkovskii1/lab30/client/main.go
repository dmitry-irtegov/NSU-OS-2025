package main

import (
	"log"
	"net"
)

const (
	socketPath = "/tmp/ex.sock"
)

func main() {

	message := "Golang Aura"
	conn, err := net.Dial("unix", socketPath)
	if err != nil {
		log.Fatal(err)
	}
	defer func() {
		_ = conn.Close()
	}()
	_, err = conn.Write([]byte(message))
	if err != nil {
		log.Fatal(err)
	}
}
