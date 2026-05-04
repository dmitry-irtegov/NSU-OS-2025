package list

import "sync"

type List struct {
	head    *node
	headMtx *sync.RWMutex
	size    int
}
