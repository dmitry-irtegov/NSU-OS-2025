package main

import (
	"fmt"
	"net"
	"os"
	"server/models"
)

func main() {
	message := "Hello, Gophers!"
	conn, err := net.Dial("unix", models.SocketAddress)
	if err != nil {
		fmt.Println(err)
		os.Exit(models.ExitFailure)
	}
	defer func() {
		_ = conn.Close()
	}()
	_, err = conn.Write([]byte(message))
	if err != nil {
		fmt.Println(err)
		os.Exit(models.ExitFailure)
	}
}
