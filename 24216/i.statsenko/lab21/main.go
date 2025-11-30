package main

import (
	"fmt"
	"os"
	"os/signal"
	"sync/atomic"
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
			atomic.AddInt32(&count, 1)
			fmt.Print('\a')
		case syscall.SIGQUIT:
			fmt.Println("\nResult: ", atomic.LoadInt32(&count))
			os.Exit(0)
		}
	}
}
