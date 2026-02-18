package main

import (
	"fmt"
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

func handleClient(state *StateMachine) {
	var err error
	defer func() {
		if err != nil {
			fmt.Println(err.Error())
		}
	}()

	fmt.Printf("Reading request from client...\n")
	err = state.handleClientRead()
	if err != nil {
		return
	}

	ok := state.tryCache()
	if ok {
		fmt.Printf("Writing response to client...\n")
		err = state.writeToClient()
		if err != nil {
			return
		}

	} else {
		fmt.Printf("Connecting to site with url: %s\n", state.url)
		err = state.connectToSite()
		if err != nil {
			return
		}

		fmt.Printf("Write request to site...\n")
		err = state.writeRequestToSite()
		if err != nil {
			return
		}

		fmt.Printf("Reading response from site...\n")
		err = state.readResponseFromSite()
		if err != nil {
			return
		}

		fmt.Printf("Writing response to client...\n")
		err = state.writeToClient()
		if err != nil {
			return
		}
	}
}

func main() {
	serverFd := createServer(8080)
	defer syscall.Close(serverFd)

	cache := NewCache()

	for {
		clientFd, _, err := syscall.Accept(serverFd)
		if err != nil {
			panic(err)
		}

		fmt.Printf("New client: %d has been connected\n", clientFd)
		go func() {
			handleClient(&StateMachine{clientFd: clientFd, cache: cache})
			defer func() {
				if clientFd != -1 {
					syscall.Close(clientFd)
					fmt.Printf("Client: %d has been disconnected\n", clientFd)
				}
			}()
		}()
	}
}
