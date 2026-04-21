package server

type cacheKey struct {
	host     string
	resource string
}

type cacheEntry struct {
	data    []byte
	mime    string
	isReady bool
	waiters []*clientSession
}

type cache struct {
	entries map[cacheKey]*cacheEntry
}
