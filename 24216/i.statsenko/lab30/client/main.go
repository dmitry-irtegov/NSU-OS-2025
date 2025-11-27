package main

import (
	"fmt"
	"net"
	"server/models"
)

func main() {
	message := "Hello, Gophers!"
	conn, err := net.Dial("unix", models.SocketAddress)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer func() {
		_ = conn.Close()
	}()
	_, err = conn.Write([]byte(message))
	if err != nil {
		fmt.Println(err)
		return
	}
}
