package server

import (
	"waldeproxy/internal/cache"
	"waldeproxy/internal/threadsafelogger"
)

var (
	port         = 8080
	serverIPAddr = [4]byte{0, 0, 0, 0}
)

type Server struct {
	socketfd     int
	respCache    *cache.Cache
	errLogger    *threadsafelogger.ErrLogger
	normalLogger *threadsafelogger.NormalLogger
}
