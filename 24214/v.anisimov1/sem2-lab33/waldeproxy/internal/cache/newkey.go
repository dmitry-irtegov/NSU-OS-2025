package cache

func NewCacheKey(host, resource string) *CacheKey {
	return &CacheKey{
		host:     host,
		resource: resource,
	}
}

func (ck *CacheKey) GetHost() string {
	return ck.host
}
