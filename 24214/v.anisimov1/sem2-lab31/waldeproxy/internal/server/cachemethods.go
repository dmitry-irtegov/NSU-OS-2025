package server

import (
	"fmt"
	"strconv"
	"strings"
)

func initCache() *cache {
	return &cache{entries: make(map[cacheKey]*cacheEntry)}
}

func (c *cache) get(key *cacheKey) (*cacheEntry, error) {
	if entry, ok := c.entries[*(key)]; ok {
		return entry, nil
	}

	return nil, fmt.Errorf("there is no entry for the passed key")
}

func (c *cache) putResponse(key *cacheKey, data []byte) {
	entry, ok := c.entries[*key]
	if !ok {
		return
	}
	entry.waiters = nil

	if responseValidCheck(data) {
		entry.data = data
		entry.mime = extractMIME(data)
		entry.isReady = true
	} else {
		delete(c.entries, *key)
	}
}

func responseValidCheck(response []byte) bool {
	stringResp := string(response)
	lines := strings.Split(stringResp, "\r\n")
	if len(lines) < 1 {
		return false
	}
	startLine := lines[0]
	tokens := strings.Split(startLine, " ")
	if len(tokens) < 3 {
		return false
	}

	if scode, err := strconv.Atoi(tokens[1]); err != nil {
		return false
	} else {
		return scode == 200
	}
}

func extractMIME(response []byte) string {
	stringResp := string(response)
	lines := strings.Split(stringResp, "\r\n")
	for _, line := range lines {
		lowline := strings.ToLower(line)
		if strings.Contains(lowline, "content-type:") {
			tokens := strings.SplitN(lowline, " ", 2)

			if len(tokens) != 2 {
				return ""
			}

			return strings.Trim(tokens[1], " \r\n")
		}
	}
	return ""
}

func (e *cacheEntry) getData() ([]byte, error) {
	if e.isReady {
		dataCopy := make([]byte, len(e.data))
		copy(dataCopy, e.data)
		return dataCopy, nil
	}

	return nil, fmt.Errorf("data is not valid")
}
