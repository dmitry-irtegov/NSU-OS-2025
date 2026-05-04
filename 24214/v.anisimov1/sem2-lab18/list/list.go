package list

import "sync"

type List struct {
	head    *node
	headMtx *sync.Mutex
	size    int
}
