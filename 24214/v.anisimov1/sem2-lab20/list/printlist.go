package list

import "fmt"

func (l *List) PrintList() {
	l.headMtx.RLock()
	node := l.head
	node.rlock()
	l.headMtx.RUnlock()

	for idx := 0; node != nil; idx++ {
		fmt.Printf("Node: %d | Val: %s\n", idx, node.value())

		nextNode := node.next()
		nextNode.rlock()
		node.runlock()
		node = nextNode
	}
}
