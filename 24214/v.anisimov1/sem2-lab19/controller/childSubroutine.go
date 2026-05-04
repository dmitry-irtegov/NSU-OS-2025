package controller

import (
	"sem2-lab18/list"
	"sync"
	"time"
)

func childSubroutine(l *list.List, toChild <-chan struct{}, wg *sync.WaitGroup) {
	defer wg.Done()
	if l == nil {
		return
	}
	for {
		select {
		case <-time.After(5 * time.Second):
			l.BubbleSort()
		case <-toChild:
			return
		}
	}
}
