package cache

func (e *CacheEntry) getData() []byte {
	e.mux.RLock()
	defer e.mux.RUnlock()

	if e.isRelevant {
		dataCopy := make([]byte, len(e.data))
		copy(dataCopy, e.data)
		return dataCopy
	} else {
		return nil
	}
}

func (e *CacheEntry) setMime(mime string) {
	e.mux.Lock()
	defer e.mux.Unlock()

	e.mime = mime
}

func (e *CacheEntry) loadData(data []byte) {
	e.mux.Lock()
	defer e.mux.Unlock()

	dataCopy := make([]byte, len(data))
	copy(dataCopy, data)
	e.data = dataCopy
}

func (e *CacheEntry) setRelevant() {
	e.mux.Lock()
	defer e.mux.Unlock()

	e.isRelevant = true
}
