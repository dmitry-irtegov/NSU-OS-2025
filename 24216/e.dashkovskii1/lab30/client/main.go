package main

import (
	"fmt"
	"log"
	"net"
)

const (
	unixSocketFile = "/tmp/ex.sock"
	textMessage    = "Golang Aura"
)

func main() {
	fmt.Println("=== Unix Socket Client ===")
	fmt.Printf("Connecting to: %s\n", unixSocketFile)

	connection, err := net.Dial("unix", unixSocketFile)
	if err != nil {
		log.Fatal("Connection failed:", err)
	}
	defer connection.Close()

	fmt.Println("Connected successfully!")
	fmt.Printf("Sending message: %s\n", textMessage)

	bytesWritten, err := connection.Write([]byte(textMessage))
	if err != nil {
		log.Fatal("Write error:", err)
	}

	fmt.Printf("Sent %d bytes\n", bytesWritten)
	fmt.Println("Message delivered!")
}
