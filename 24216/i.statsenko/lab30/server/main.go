package main

import (
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"os/signal"
	"server/models"
	"strings"
	"syscall"
)

func main() {
	_ = os.Remove(models.SocketAddress)
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGTERM, syscall.SIGINT)

	server, err := net.Listen("unix", models.SocketAddress)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer func() {
		_ = server.Close()
		_ = os.Remove(models.SocketAddress)
	}()
	go run(server)
	<-quit
	fmt.Println("Graceful shutdown")
}

func run(server net.Listener) {
	for {
		conn, err := server.Accept()
		switch {
		case errors.Is(err, net.ErrClosed):
			return
		case err != nil:
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
	result := make([]rune, 0)
	readed := 0
	for readed < models.MaxLenMessage {
		n, err := conn.Read(buf)
		if err == io.EOF {
			break
		}
		if err != nil {
			return err
		}
		data := []rune(string(buf[:n]))
		countSymbols := len(data)
		if readed+countSymbols > models.MaxLenMessage {
			countSymbols = models.MaxLenMessage - readed
		}
		result = append(result, data[:countSymbols]...)
		readed += countSymbols
	}
	newStr := strings.ToUpper(string(result))
	if readed == models.MaxLenMessage {
		fmt.Printf("toUpper first %d symbols: %s\n", models.MaxLenMessage, newStr)
	} else {
		fmt.Printf("toUpper message: %s\n", newStr)
	}
	return nil
}
