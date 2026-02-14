package list

import (
	"fmt"
	"sync"
	"time"
)

type node struct {
	prev  *node
	next  *node
	value string
}

type LinkedList struct {
	head  *node
	mtx   sync.Mutex
	count int
}

func NewLinkedList() *LinkedList {
	return &LinkedList{}
}

func (l *LinkedList) AddToEnd(value string) {
	l.mtx.Lock()
	defer l.mtx.Unlock()

	if l.count == 0 {
		nd := &node{value: value}
		nd.next = nd
		nd.prev = nd
		l.count++
		l.head = nd
		return
	}

	tail := l.head.prev

	newNode := &node{
		next:  l.head,
		prev:  tail,
		value: value,
	}

	tail.next = newNode
	l.head.prev = newNode

	l.count++
}

func (l *LinkedList) Print() {
	l.mtx.Lock()
	defer l.mtx.Unlock()
	fmt.Println("=== Elements in list ===")
	if l.count == 0 {
		return
	}
	cur := l.head
	for i := 0; i < l.count; i++ {
		fmt.Println(cur.value)
		cur = cur.next
	}
}

func (l *LinkedList) Sort(duration time.Duration) {
	for {
		time.Sleep(duration)
		l.mtx.Lock()

		if l.count < 2 {
			l.mtx.Unlock()
			continue
		}
		for i := 0; i < l.count; i++ {
			cur := l.head
			for j := 0; j < l.count-1; j++ {
				next := cur.next
				if cur.value > next.value {
					l.swap(cur, next)
				} else {
					cur = cur.next
				}
			}
		}
		l.mtx.Unlock()
	}
}

func (l *LinkedList) swap(n1, n2 *node) {
	if l.head == n1 {
		l.head = n2
	} else if l.head == n2 {
		l.head = n1
	}

	left := n1.prev
	right := n2.next

	n2.prev = left
	n2.next = n1

	n1.prev = n2
	n1.next = right

	left.next = n2
	right.prev = n1
}
