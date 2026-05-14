package controller

import (
	"fmt"
	"os"
	"strconv"
	"waldeproxy/internal/server"
)

func Run() {
	args := os.Args

	if len(args) != 2 {
		fmt.Println("uncorrect usage")
		return
	}

	if workerNumbers, err := strconv.Atoi(args[1]); err != nil {
		fmt.Fprintf(os.Stderr, "cannot parse %v\n", err)
	} else {
		server, err := server.NewServer(workerNumbers)

		if err != nil {
			fmt.Fprintf(os.Stderr, "cannot create server: %v", err)
			return
		}

		if err := server.TurnOn(1000); err != nil {
			fmt.Fprintf(os.Stderr, "cannot turn on the server: %v", err)
			server.Destroy()
			return
		}

		server.Work()
	}
}
