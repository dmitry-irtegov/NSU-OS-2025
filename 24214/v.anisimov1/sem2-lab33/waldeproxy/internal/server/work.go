package server

import (
	"waldeproxy/internal/worker"

	"golang.org/x/sys/unix"
)

func (s *Server) Work() {
	s.normalLogger.Println("turning on...")

	serverPollingArea := []unix.PollFd{unix.PollFd{
		Fd:      int32(s.socket),
		Events:  unix.POLLIN,
		Revents: 0,
	},
	}

	for id := range s.workerAmount {
		worker := worker.NewWorker(
			id,
			s.socketChannel,
			s.respCache,
		)

		go worker.Run()

		s.normalLogger.Printf("worker with ID [%d] has been turned on...\n", id)
	}

	for {
		n, err := unix.Poll(serverPollingArea, -1)

		if err != nil {
			s.errLogger.Printf("cannot poll the socket: %v\n", err)
			continue
		}

		if n > 0 {
			client, _, err := unix.Accept(s.socket)

			if err != nil {
				s.errLogger.Printf("cannot accept client: %v\n", err)
				continue
			}

			if err := unix.SetNonblock(client, true); err != nil {
				unix.Close(client)
				s.errLogger.Printf("cannot set clinent's socket to non-block mode: %v\n", err)
				continue
			}

			s.socketChannel <- client

			serverPollingArea[0].Revents = 0
		}
	}
}
