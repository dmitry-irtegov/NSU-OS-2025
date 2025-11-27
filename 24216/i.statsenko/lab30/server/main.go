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
	for {
		conn, err := server.Accept()
		if err != nil {
			fmt.Println(err)
			continue
		}
		err = handleConnection(conn)
		if err != nil {
			fmt.Println(err)
		}
		_ = conn.Close()
	}
}

func handleConnection(conn net.Conn) error {
	buf := make([]byte, models.SizeBuffer)
	result := make([]byte, 0)
	readed := 0
	for readed < models.MaxLenMessage {
		n, err := conn.Read(buf)
		if err == io.EOF {
			break
		}
		if err != nil {
			return err
		}
		if readed+n > models.MaxLenMessage {
			n = models.MaxLenMessage - readed
		}
		readed += n
		result = append(result, buf[:n]...)
	}
	newStr := strings.ToUpper(string(result))
	if readed == models.MaxLenMessage {
		fmt.Printf("toUpper first %d bytes: %s\n", models.MaxLenMessage, newStr)
	} else {
		fmt.Printf("toUpper message: %s\n", newStr)
	}
	return nil
}
