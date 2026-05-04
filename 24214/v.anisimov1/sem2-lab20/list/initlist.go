package list

import (
	"sync"
)

func NewList() *List {
	return &List{
		head:    nil,
		headMtx: &sync.RWMutex{},
		size:    0,
	}
}
