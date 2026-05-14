package server

import (
	"log"
	"waldeproxy/internal/cache"
)

type Server struct {
	socket        int
	workerAmount  int
	respCache     *cache.Cache
	socketChannel chan int
	errLogger     *log.Logger
	normalLogger  *log.Logger
}
