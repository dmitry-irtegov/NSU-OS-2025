package worker

import (
	"fmt"
	"log"
	"os"
	"waldeproxy/internal/cache"
	"waldeproxy/internal/client"

	"golang.org/x/sys/unix"
)

func NewWorker(
	id int,
	clientSource <-chan int,
	cache *cache.Cache,
) *Worker {
	return &Worker{
		id:           id,
		clientSourse: clientSource,
		pollArea:     make([]unix.PollFd, 0),
		clientMap:    make(map[int]*client.ClientContext),
		upstreamMap:  make(map[int]*client.ClientContext),
		errLogger:    log.New(os.Stderr, fmt.Sprintf("worker [%d] ERROR >>> ", id), 0),
		normalLogger: log.New(os.Stdout, fmt.Sprintf("worker [%d] report >>> ", id), 0),
		cache:        cache,
	}
}
