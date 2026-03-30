package main

import (
	"sync"
	"time"
)

type CacheEntry struct {
	Data      []byte
	CreatedAt time.Time
}

type Cache struct {
	mu      sync.RWMutex
	entries map[string]*CacheEntry
}

func NewCache() *Cache {
	return &Cache{entries: make(map[string]*CacheEntry)}
}

func (c *Cache) Get(key string) (*CacheEntry, bool) {
	c.mu.RLock()
	defer c.mu.RUnlock()
	e, ok := c.entries[key]
	return e, ok
}

func (c *Cache) Set(key string, data []byte) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.entries[key] = &CacheEntry{Data: data, CreatedAt: time.Now()}
}
