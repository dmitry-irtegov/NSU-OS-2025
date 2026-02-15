package main

import "time"

type CacheEntry struct {
	Data      []byte
	Timestamp time.Time
}

var cache = make(map[string]CacheEntry)
