package main

import (
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"strings"
)

const (
	unixSocketFile = "/tmp/ex.sock"
	messageBuffer  = 2048
)

func main() {
	os.Remove(unixSocketFile)

	fmt.Printf("Listening on: %s\n", unixSocketFile)

	listener, err := net.Listen("unix", unixSocketFile)
	if err != nil {
		log.Fatal("Listen error:", err)
	}
	defer listener.Close()

	fmt.Println("Waiting for connection...")

	connection, err := listener.Accept()
	if err != nil {
		log.Fatal("Accept error:", err)
	}
	defer connection.Close()

	fmt.Println("Client connected!")

	buffer := make([]byte, messageBuffer)
	bytesRead, err := connection.Read(buffer)
	if err != nil && err != io.EOF {
		log.Fatal("Read error:", err)
	}

	receivedData := buffer[:bytesRead]
	upperCaseText := strings.ToUpper(string(receivedData))

	fmt.Printf("Uppercase: %s\n", upperCaseText)
}
