package worker

import (
	"log"
	"waldeproxy/internal/cache"
	"waldeproxy/internal/client"

	"golang.org/x/sys/unix"
)

type Worker struct {
	id           int
	clientSourse <-chan int
	pollArea     []unix.PollFd
	clientMap    map[int]*client.ClientContext
	upstreamMap  map[int]*client.ClientContext
	errLogger    *log.Logger
	normalLogger *log.Logger
	cache        *cache.Cache
}
