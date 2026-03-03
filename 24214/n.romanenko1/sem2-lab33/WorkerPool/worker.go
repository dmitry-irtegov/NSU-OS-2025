package workerpool

import (
	"fmt"
	cache "proxy/Cache"
	statemachine "proxy/StateMachine"

	"golang.org/x/sys/unix"
)

type Worker struct {
	id     int
	max    int
	states map[int]*statemachine.StateMachine
	allFds []int
	cache  *cache.Cache
}

func NewWorker(id int, max int, cache *cache.Cache) *Worker {
	fmt.Printf("New worker with id: %d\n", id)
	states := make(map[int]*statemachine.StateMachine)
	allFds := make([]int, 0)
	return &Worker{
		id:     id,
		max:    max,
		states: states,
		allFds: allFds,
		cache:  cache,
	}
}

func (w *Worker) Start(tasks chan Task) {
	fmt.Printf("New worker with id: %d started\n", w.id)
	for {
		select {
		case task := <-tasks:
			fmt.Printf("New client: %d\n handle by worker with id: %d\n", task.ClientFd, w.id)
			w.states[task.ClientFd] = statemachine.NewStateMachine(task.ClientFd, w.cache)
			w.allFds = append(w.allFds, task.ClientFd)
		default:
			var readSet, writeSet unix.FdSet
			n, err := initSelect(&w.allFds, &readSet, &writeSet, w.states)
			if err != nil {
				if err != unix.EINTR {
					panic(err)
				}
			}

			if n != 0 && err == nil {
				currentFds := make([]int, len(w.allFds))
				copy(currentFds, w.allFds)

				for _, fd := range currentFds {
					if readSet.IsSet(fd) {
						if state, ok := w.states[fd]; ok {
							switch state.Stage {
							case statemachine.STAGE_READ_FROM_CLIENT:
								fmt.Println("Cliet ready to send data")
								state.HandleClientRead(&w.allFds, w.states)
							case statemachine.STAGE_READ_RESPONSE_FROM_SITE:
								fmt.Println("Site ready to send response")
								state.ProcessSite(&w.allFds, w.states) //non-blocking
							}
						}
					}

					if writeSet.IsSet(fd) {
						if state, ok := w.states[fd]; ok {
							switch state.Stage {
							case statemachine.STAGE_WRITE_REQUEST_TO_SITE:
								fmt.Println("Ready to send request to site")
								state.ProcessSite(&w.allFds, w.states) //non-blocking
							case statemachine.STAGE_WRITE_TO_CLIENT:
								fmt.Println("Ready to send response to client")
								state.ProcessSite(&w.allFds, w.states) //non-blocking
							}
						}
					}
				}
			}
		}
	}
}

func initSelect(allFds *[]int, readSet *unix.FdSet, writeSet *unix.FdSet, states map[int]*statemachine.StateMachine) (int, error) {
	readSet.Zero()
	writeSet.Zero()
	maxFd := 0

	for _, fd := range *allFds {
		if fd >= 1024 {
			continue
		}

		readSet.Set(fd)

		if state, ok := states[fd]; ok {
			if state.Stage == statemachine.STAGE_WRITE_REQUEST_TO_SITE {
				writeSet.Set(state.SiteFd)
			}

			if state.Stage == statemachine.STAGE_WRITE_TO_CLIENT {
				writeSet.Set(state.ClientFd)
			}
		}

		if fd > maxFd {
			maxFd = fd
		}
	}

	timeout := &unix.Timeval{Sec: 5, Usec: 0}
	return unix.Select(maxFd+1, readSet, writeSet, nil, timeout)
}
