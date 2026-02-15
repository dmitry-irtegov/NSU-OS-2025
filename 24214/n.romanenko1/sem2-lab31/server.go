package main

import (
	"fmt"

	"golang.org/x/sys/unix"
)

func createServer(port int) int {
	fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, unix.IPPROTO_TCP)
	if err != nil {
		panic(err)
	}

	unix.SetsockoptInt(fd, unix.SOL_SOCKET, unix.SO_REUSEADDR, 1)

	if err := unix.SetNonblock(fd, true); err != nil {
		panic(err)
	}

	addr := unix.SockaddrInet4{Port: port, Addr: [4]byte{0, 0, 0, 0}}
	if err := unix.Bind(fd, &addr); err != nil {
		panic(err)
	}

	if err := unix.Listen(fd, 128); err != nil {
		panic(err)
	}

	fmt.Println("Proxy started on port 8080")
	return fd
}

func initSelect(allFds *[]int, readSet *unix.FdSet, writeSet *unix.FdSet, states map[int]*StateMachine) (int, error) {
	readSet.Zero()
	writeSet.Zero()
	maxFd := 0

	for _, fd := range *allFds {
		if fd >= 1024 {
			continue
		}

		readSet.Set(fd)

		if state, ok := states[fd]; ok {
			if state.stage == STAGE_WRITE_REQUEST_TO_SITE {
				writeSet.Set(state.siteFd)
			}

			if state.stage == STAGE_WRITE_TO_CLIENT {
				writeSet.Set(state.clientFd)
			}
		}

		if fd > maxFd {
			maxFd = fd
		}
	}

	timeout := &unix.Timeval{Sec: 5, Usec: 0}
	return unix.Select(maxFd+1, readSet, writeSet, nil, timeout)
}

func main() {
	siteFd := createServer(8080)
	defer unix.Close(siteFd)

	states := make(map[int]*StateMachine)
	allFds := []int{siteFd}

	for {
		var readSet, writeSet unix.FdSet
		n, err := initSelect(&allFds, &readSet, &writeSet, states)

		if err != nil {
			if err != unix.EINTR {
				panic(err)
			}
		}

		if n != 0 && err == nil {
			currentFds := make([]int, len(allFds))
			copy(currentFds, allFds)

			for _, fd := range currentFds {
				if readSet.IsSet(fd) {
					if fd == siteFd {
						nfd, _, err := unix.Accept(siteFd) //non-blocking
						if err != nil {
							panic(err)
						}

						unix.SetNonblock(nfd, true)

						states[nfd] = &StateMachine{clientFd: nfd, stage: STAGE_READ_FROM_CLIENT}
						allFds = append(allFds, nfd)
						fmt.Printf("New client: %d\n", nfd)

					} else if state, ok := states[fd]; ok {
						switch state.stage {
						case STAGE_READ_FROM_CLIENT:
							fmt.Println("Cliet ready to send data")
							state.handleClientRead(&allFds, states)
						case STAGE_READ_RESPONSE_FROM_SITE:
							fmt.Println("Site ready to send response")
							state.processSite(&allFds, states) //non-blocking
						}
					}
				}

				if writeSet.IsSet(fd) {
					if state, ok := states[fd]; ok {
						switch state.stage {
						case STAGE_WRITE_REQUEST_TO_SITE:
							fmt.Println("Ready to send request to site")
							state.processSite(&allFds, states) //non-blocking
						case STAGE_WRITE_TO_CLIENT:
							fmt.Println("Ready to send response to client")
							state.processSite(&allFds, states) //non-blocking
						}
					}
				}
			}
		}
	}
}
