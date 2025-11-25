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
	socketPath    = "/tmp/ex.sock"
	maxLenMessage = 2048
)

func main() {

	_ = os.Remove(socketPath)

	server, err := net.Listen("unix", socketPath)
	if err != nil {
		log.Fatal(err)
	}
	defer func() {
		_ = server.Close()
	}()

	conn, err := server.Accept()
	if err != nil {
		log.Fatal(err)
	}
	defer func() {
		_ = conn.Close()
	}()

	buf := make([]byte, maxLenMessage)
	n, err := conn.Read(buf)
	if err != nil && err != io.EOF {
		log.Fatal(err)
	}
	data := buf[:n]
	newStr := strings.ToUpper(string(data))
	fmt.Println("toUpper message: ", newStr)
}
