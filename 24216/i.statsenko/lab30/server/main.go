package main

import (
	"fmt"
	"io"
	"net"
	"os"
	"server/models"
	"strings"
)

func main() {
	_ = os.Remove(models.SocketAddress)

	server, err := net.Listen("unix", models.SocketAddress)
	if err != nil {
		fmt.Println(err)
		os.Exit(models.ExitFailure)
	}
	defer func() {
		_ = server.Close()
	}()

	conn, err := server.Accept()
	if err != nil {
		fmt.Println(err)
		os.Exit(models.ExitFailure)
	}
	defer func() {
		_ = conn.Close()
	}()

	buf := make([]byte, models.MaxLenMessage)
	n, err := conn.Read(buf)
	if err != nil && err != io.EOF {
		fmt.Println(err)
		os.Exit(models.ExitFailure)
	}
	data := buf[:n]
	newStr := strings.ToUpper(string(data))
	fmt.Println("toUpper message: ", newStr)
}
