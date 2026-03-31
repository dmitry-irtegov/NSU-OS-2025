package main

import (
	"fmt"
	"sync"
)

type List struct {
	head *Node
	tail *Node
	mtx  *sync.Mutex
}

type Node struct {
	next *Node
	val  string
}

func InitList() *List {
	return &List{head: nil, tail: nil, mtx: &sync.Mutex{}}
}

func (l *List) pushBack(val string) {
	if l == nil {
		return
	}

	node := &Node{next: nil, val: val}

	l.mtx.Lock()
	if l.head == nil && l.tail == nil {
		l.head = node
		l.tail = node
	} else {
		l.tail.next = node
		l.tail = node
	}
	l.mtx.Unlock()
}

func (l *List) printList() {
	if l == nil {
		return
	}

	l.mtx.Lock()
	for node, idx := l.head, 0; node != nil; node, idx = node.next, idx+1 {
		fmt.Printf("Node [%d] | Val [%s]\n", idx, node.val)
	}
	l.mtx.Unlock()
}
