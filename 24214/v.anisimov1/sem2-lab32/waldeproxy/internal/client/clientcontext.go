package client

import (
	"waldeproxy/internal/cache"
	"waldeproxy/internal/threadsafelogger"
)

type ClientContext struct {
	clientid     int
	socketFd     int
	cache        *cache.Cache
	errLogger    *threadsafelogger.ErrLogger
	normalLogger *threadsafelogger.NormalLogger
}

func InitClientContext(
	clientid int,
	socketfd int,
	cache *cache.Cache,
	errLogger *threadsafelogger.ErrLogger,
	normalLogger *threadsafelogger.NormalLogger,
) *ClientContext {
	return &ClientContext{
		clientid:     clientid,
		socketFd:     socketfd,
		cache:        cache,
		errLogger:    errLogger,
		normalLogger: normalLogger,
	}
}
