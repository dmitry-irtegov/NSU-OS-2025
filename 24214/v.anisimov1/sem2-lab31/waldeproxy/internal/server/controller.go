package server

import (
	"log"
	"os"
)

func Run() {
	errLogger := log.New(os.Stderr, ">>> WARNING: ", 0)

	server, err := createServer(1000)

	if err != nil {
		errLogger.Printf("create server: %v", err)
		return
	}

	if err := server.turnon(); err != nil {
		errLogger.Printf("cannot turnon the server: %v", err)
		server.destroy()
		return
	}

	for {
		if err := server.iteration(); err != nil {
			errLogger.Printf("iteration error: %v", err)
			server.destroy()
			return
		}
	}
}
