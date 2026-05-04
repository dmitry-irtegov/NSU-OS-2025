package list

import "sync"

type node struct {
	val string
	nxt *node
	mtx *sync.RWMutex
}

func (n *node) lock() {
	if n != nil {
		n.mtx.Lock()
	}
}

func (n *node) unlock() {
	if n != nil {
		n.mtx.Unlock()
	}
}

func (n *node) rlock() {
	if n != nil {
		n.mtx.RLock()
	}
}

func (n *node) runlock() {
	if n != nil {
		n.mtx.RUnlock()
	}
}

func (n *node) value() string {
	if n != nil {
		return n.val
	}
	return ""
}

func (n *node) next() *node {
	if n != nil {
		return n.nxt
	}

	return nil
}

func (n *node) setValue(val string) {
	if n != nil {
		n.val = val
	}
}

func (n *node) setNext(next *node) {
	if n != nil {
		n.nxt = next
	}
}
