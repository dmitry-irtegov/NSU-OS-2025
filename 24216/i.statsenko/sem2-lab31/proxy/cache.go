package proxy

import "sync"

type Cache struct {
	mtx  sync.RWMutex
	data map[string][]byte
}

func NewCache() *Cache {
	return &Cache{data: make(map[string][]byte)}
}

func (c *Cache) Get(key string) ([]byte, bool) {
	c.mtx.RLock()
	defer c.mtx.RUnlock()
	v, ok := c.data[key]
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
	c.mtx.Lock()
	defer c.mtx.Unlock()
	c.data[key] = cp
}
