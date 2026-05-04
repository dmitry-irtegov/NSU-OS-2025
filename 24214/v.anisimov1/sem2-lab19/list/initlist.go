package list

import (
	"sync"
)

func NewList() *List {
	return &List{
		head:    nil,
		headMtx: &sync.Mutex{},
		size:    0,
	}
}
