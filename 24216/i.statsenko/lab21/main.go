package main

import (
	"fmt"
	"os"
	"os/signal"
	"syscall"
)

func main() {
	var count int32
	signals := make(chan os.Signal, 1)

	signal.Notify(signals, syscall.SIGINT, syscall.SIGQUIT)
	fmt.Println("Send SIGINT to increment counter, SIGQUIT to exit")
	for sig := range signals {
		switch sig {
		case syscall.SIGINT:
			count++
			fmt.Printf("%c", '\a')
		case syscall.SIGQUIT:
			fmt.Println("\nResult:", count)
			os.Exit(0)
		}
	}
}
