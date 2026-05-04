package list

import (
	"fmt"
	"sync"
)

func (l *List) Add(st string) error {
	if l == nil {
		return fmt.Errorf("nil list")
	}

	l.headMtx.Lock()
	if l.head == nil {
		head := &node{val: st, nxt: nil, mtx: &sync.Mutex{}}
		l.head = head
		l.size = 1
	} else {
		newNode := &node{val: st, nxt: l.head, mtx: &sync.Mutex{}}
		l.size++
		l.head = newNode
	}
	l.headMtx.Unlock()

	return nil
}
