package main

import (
	"sync"
	"time"
)

type CacheEntry struct {
	Data      []byte
	Timestamp time.Time
}

type Cache struct {
	entries map[string]CacheEntry
	mutex   sync.RWMutex
}

func NewCache() *Cache {
	return &Cache{
		entries: make(map[string]CacheEntry),
	}
}

func (c *Cache) Get(url string) (CacheEntry, bool) {
	c.mutex.RLock()
	defer c.mutex.RUnlock()
	entry, exists := c.entries[url]
	return entry, exists
}

func (c *Cache) Set(url string, data []byte) {
	c.mutex.Lock()
	defer c.mutex.Unlock()
	c.entries[url] = CacheEntry{Data: data, Timestamp: time.Now()}
}
