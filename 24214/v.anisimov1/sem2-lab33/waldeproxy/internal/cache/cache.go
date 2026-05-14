package cache

import (
	"errors"
	"fmt"
	"sync"
)

var (
	EmptyDoesNotExistError = errors.New("empty does not exist error")
	NotEnoughData          = errors.New("try to read data again")
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

	if _, ok := c.entries[*(key)]; !ok {
		c.entries[*(key)] = &CacheEntry{
			data:   make([]byte, 0),
			mime:   "",
			isFull: false,
			mux:    &sync.RWMutex{},
		}

		return nil
	} else {
		return fmt.Errorf("there is an entry already")
	}
}

func (c *Cache) PutDataIntoCache(
	key *CacheKey,
	data []byte,
) error {
	c.access.RLock()
	defer c.access.RUnlock()

	if entry, ok := c.entries[*(key)]; ok {
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

func (c *Cache) GetData(key *CacheKey, offset int) ([]byte, error) {
	c.access.RLock()
	defer c.access.RUnlock()

	if entry, ok := c.entries[*(key)]; ok {
		if data, err := entry.getData(offset); err != NotEnoughData {
			return data, nil
		} else {
			return nil, NotEnoughData
		}
	} else {
		return nil, EmptyDoesNotExistError
	}
}

func (c *Cache) GetFull(key *CacheKey) (bool, error) {
	c.access.RLock()
	defer c.access.RUnlock()

	if entry, ok := c.entries[*(key)]; ok {
		return entry.getFull(), nil
	} else {
		return false, EmptyDoesNotExistError
	}
}

func (c *Cache) AcceptEntry(key *CacheKey) error {
	c.access.Lock()
	defer c.access.Unlock()

	if entry, ok := c.entries[*(key)]; ok {
		entry.acceptEntry()

		return nil
	} else {
		return EmptyDoesNotExistError
	}
}

func (c *Cache) SetMime(key *CacheKey, mime string) error {
	c.access.Lock()
	defer c.access.Unlock()

	if entry, ok := c.entries[*(key)]; ok {
		entry.setMime(mime)

		return nil
	} else {
		return EmptyDoesNotExistError
	}
}

func (c *Cache) HasNewData(key *CacheKey, offset int) (bool, error) {
	c.access.RLock()
	defer c.access.RUnlock()

	if entry, ok := c.entries[*(key)]; ok {
		return entry.hasNewData(offset), nil
	} else {
		return false, EmptyDoesNotExistError
	}
}
