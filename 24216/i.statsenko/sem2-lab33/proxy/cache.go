package proxy

import "sync"

type Cache struct {
	mu    sync.RWMutex
	items map[string][]byte
}

func NewCache() *Cache {
	return &Cache{items: make(map[string][]byte)}
}

func (c *Cache) Get(key string) ([]byte, bool) {
	c.mu.RLock()
	defer c.mu.RUnlock()
	v, ok := c.items[key]
	if !ok {
		return nil, false
	}
	cp := make([]byte, len(v))
	copy(cp, v)
	return cp, true
}

func (c *Cache) Set(key string, data []byte) {
	cp := make([]byte, len(data))
	copy(cp, data)
	c.mu.Lock()
	defer c.mu.Unlock()
	c.items[key] = cp
}
