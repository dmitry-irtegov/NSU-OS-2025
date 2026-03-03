package cache

import (
	"sync"
	"time"
)

type CacheEntry struct {
	Data      []byte
	Timestamp time.Time
}

type Cache struct {
	data map[string]CacheEntry
	mu   *sync.RWMutex
}

func NewCache() *Cache {
	return &Cache{
		data: make(map[string]CacheEntry),
		mu:   &sync.RWMutex{},
	}
}

func (c *Cache) Get(key string) (CacheEntry, bool) {
	c.mu.RLock()
	defer c.mu.RUnlock()

	entry, exists := c.data[key]
	if !exists {
		return CacheEntry{}, false
	}
	return entry, true
}

func (c *Cache) Set(key string, entry CacheEntry) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.data[key] = entry
}
