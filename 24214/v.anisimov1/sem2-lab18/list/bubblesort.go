package list

func (l *List) BubbleSort() {
	for {
		noSwaps := true

		l.headMtx.Lock()

		current := l.head
		current.lock()
		next := current.next()
		next.lock()

		if next != nil && next.value() < current.value() {
			noSwaps = false
			l.head = next
			current.setNext(next.next())
			next.setNext(current)

			current, next = next, current
		}

		prev := current
		current = next
		next = next.next()
		next.lock()

		l.headMtx.Unlock()

		for next != nil {
			if next.value() < current.value() {
				noSwaps = false
				prev.setNext(next)
				current.setNext(next.next())
				next.setNext(current)

				current, next = next, current
			}

			prev.unlock()
			prev = current
			current = next
			next = next.next()
			next.lock()
		}

		prev.unlock()
		current.unlock()

		if noSwaps {
			return
		}
	}
}
