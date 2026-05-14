package client

import "waldeproxy/internal/cache"

type ClientContext struct {
	ClientFd   int
	UpstreamFd int

	ClientPollIdx   int
	UpstreamPollIdx int

	State ClientState

	Buffer      []byte
	PayloadSize int
	WroteBytes  int

	Key *cache.CacheKey

	Waiting bool

	HeaderProcessed bool
	WritingToCache  bool
}
