package cache

func (e *CacheEntry) getData(offset int) ([]byte, error) {
	e.mux.RLock()
	defer e.mux.RUnlock()

	if offset >= len(e.data) {
		return nil, NotEnoughData
	}

	result := make([]byte, len(e.data)-offset)
	copy(result, e.data[offset:])
	return result, nil
}

func (e *CacheEntry) loadData(data []byte) {
	e.mux.Lock()
	defer e.mux.Unlock()

	e.data = append(e.data, data...)
}

func (e *CacheEntry) acceptEntry() {
	e.mux.Lock()
	defer e.mux.Unlock()

	e.isFull = true
}

func (e *CacheEntry) setMime(mime string) {
	e.mux.Lock()
	defer e.mux.Unlock()

	e.mime = mime
}

func (e *CacheEntry) getFull() bool {
	e.mux.RLock()
	defer e.mux.RUnlock()

	return e.isFull
}

func (e *CacheEntry) hasNewData(offset int) bool {
	e.mux.RLock()
	defer e.mux.RUnlock()

	return len(e.data)-offset > 0
}
