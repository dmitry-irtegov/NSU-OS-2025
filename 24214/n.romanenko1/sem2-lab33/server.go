package main

import (
	"fmt"
	cache "proxy/Cache"
	workerpool "proxy/WorkerPool"
	"syscall"
)

func createServer(port int) int {
	fd, err := syscall.Socket(syscall.AF_INET, syscall.SOCK_STREAM, syscall.IPPROTO_TCP)
	if err != nil {
		panic(err)
	}

	syscall.SetsockoptInt(fd, syscall.SOL_SOCKET, syscall.SO_REUSEADDR, 1)

	addr := syscall.SockaddrInet4{Port: port, Addr: [4]byte{0, 0, 0, 0}}
	if err := syscall.Bind(fd, &addr); err != nil {
		panic(err)
	}

	if err := syscall.Listen(fd, 128); err != nil {
		panic(err)
	}

	fmt.Println("Proxy started on port 8080")
	return fd
}

func main() {
	serverFd := createServer(8080)
	defer syscall.Close(serverFd)

	cache := cache.NewCache()
	tasks := make(chan workerpool.Task, 1024)
	workerPool := workerpool.NewWorkerPool(tasks, 4, cache)
	workerPool.Start()

	for {
		clientFd, _, err := syscall.Accept(serverFd)
		if err != nil {
			panic(err)
		}

		fmt.Printf("New client: %d has been connected\n", clientFd)
		tasks <- workerpool.Task{ClientFd: clientFd}
	}
}
