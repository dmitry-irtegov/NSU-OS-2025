package cache

import "sync"

type CacheKey struct {
	host     string
	resource string
}

type CacheEntry struct {
	data   []byte
	mime   string
	isFull bool
	mux    *sync.RWMutex
}

type Cache struct {
	entries map[CacheKey]*CacheEntry
	access  *sync.RWMutex
}
