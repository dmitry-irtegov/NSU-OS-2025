package list

import "fmt"

func (l *List) PrintList() {
	l.headMtx.Lock()
	node := l.head
	node.lock()
	l.headMtx.Unlock()

	for idx := 0; node != nil; idx++ {
		fmt.Printf("Node: %d | Val: %s\n", idx, node.value())

		nextNode := node.next()
		nextNode.lock()
		node.mtx.Unlock()
		node = nextNode
	}
}
