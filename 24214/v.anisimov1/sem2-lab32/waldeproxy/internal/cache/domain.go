package cache

import "sync"

type CacheKey struct {
	Host     string
	Resource string
}

type CacheEntry struct {
	data       []byte
	mime       string
	isRelevant bool
	mux        *sync.RWMutex
}

type Cache struct {
	entries map[CacheKey]*CacheEntry
	access  *sync.RWMutex
}
