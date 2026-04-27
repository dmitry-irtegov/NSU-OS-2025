package cache

import (
	"errors"
	"fmt"
	"sync"
)

var (
	IsNotRelevantNowError  = errors.New("try again to check relevance")
	EmptyDoesNotExistError = errors.New("empty does not exist error")
)

func InitCache() *Cache {
	return &Cache{
		entries: make(map[CacheKey]*CacheEntry),
		access:  &sync.RWMutex{},
	}
}

func (c *Cache) IsEntryExists(key *CacheKey) bool {
	c.access.RLock()
	defer c.access.RUnlock()

	_, ok := c.entries[*(key)]

	return ok
}

func (c *Cache) RegisterEntry(key *CacheKey) error {
	c.access.Lock()
	defer c.access.Unlock()

	entry := &CacheEntry{
		data:       nil,
		mime:       "",
		isRelevant: false,
		mux:        &sync.RWMutex{},
	}

	if _, ok := c.entries[*(key)]; !ok {
		c.entries[*(key)] = entry
		return nil
	} else {
		return fmt.Errorf("there is an entry already")
	}
}

func (c *Cache) AddResponseToEntry(
	key *CacheKey,
	data []byte,
	mime string,
) error {
	c.access.RLock()
	defer c.access.RUnlock()

	if entry, ok := c.entries[*(key)]; ok {
		entry.setRelevant()
		entry.setMime(mime)
		entry.loadData(data)
		return nil
	} else {
		return fmt.Errorf("there isn't entry for the key")
	}
}

func (c *Cache) DeleteEntry(key *CacheKey) {
	c.access.Lock()
	defer c.access.Unlock()

	delete(c.entries, *(key))
}

func (c *Cache) GetData(key *CacheKey) ([]byte, error) {
	c.access.RLock()
	defer c.access.RUnlock()

	if entry, ok := c.entries[*(key)]; ok {
		if data := entry.getData(); data != nil {
			return data, nil
		} else {
			return nil, IsNotRelevantNowError
		}
	} else {
		return nil, EmptyDoesNotExistError
	}
}
