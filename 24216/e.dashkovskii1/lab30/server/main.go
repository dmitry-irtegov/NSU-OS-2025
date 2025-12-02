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

	fmt.Println("Waiting for connections...")
	clientNumber := 0

	for {
		connection, err := listener.Accept()
		if err != nil {
			log.Printf("Accept error: %v\n", err)
			continue
		}

		clientNumber++
		go handleClient(connection, clientNumber)
	}
}

func handleClient(connection net.Conn, clientID int) {
	defer connection.Close()

	fmt.Printf("[Client #%d] Connected\n", clientID)

	buffer := make([]byte, messageBuffer)
	bytesRead, err := connection.Read(buffer)
	if err != nil && err != io.EOF {
		log.Printf("[Client #%d] Read error: %v\n", clientID, err)
		return
	}

	receivedData := buffer[:bytesRead]
	upperCaseText := strings.ToUpper(string(receivedData))

	fmt.Printf("[Client #%d] Received: %s\n", clientID, string(receivedData))
	fmt.Printf("[Client #%d] Uppercase: %s\n", clientID, upperCaseText)
	fmt.Printf("[Client #%d] Disconnected\n", clientID)
}
