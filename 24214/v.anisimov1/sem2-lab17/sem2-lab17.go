package main

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"strings"
	"sync"
	"time"
)

var NilList = errors.New("Nil-list")
var UnInitList = errors.New("Uninitialized list")

type node struct {
	val  string
	next *node
}

type list struct {
	head *node
	size int
	mtx  *sync.Mutex
}

func (l *list) init() error {
	if l == nil {
		return NilList
	}
	l.mtx = &sync.Mutex{}
	return nil
}

func (l *list) add(st string) error {
	if l == nil {
		return NilList
	}
	if l.mtx == nil {
		return UnInitList
	}
	l.mtx.Lock()
	defer l.mtx.Unlock()
	if l.head == nil {
		head := &node{val: st, next: nil}
		l.head = head
		l.size = 1
	} else {
		newNode := &node{val: st, next: l.head}
		l.size++
		l.head = newNode
	}
	return nil
}

func (l *list) printList() error {
	if l == nil {
		return NilList
	}
	if l.mtx == nil {
		return UnInitList
	}
	l.mtx.Lock()
	defer l.mtx.Unlock()
	for currNode, idx := l.head, 0; currNode != nil; currNode, idx = currNode.next, idx+1 {
		fmt.Printf("Node: %d | Val: %s\n", idx, currNode.val)
	}
	return nil
}

func (l *list) bubbleSort() error {
	if l == nil {
		return NilList
	}
	if l.mtx == nil {
		return UnInitList
	}
	l.mtx.Lock()
	defer l.mtx.Unlock()
	for fst := l.head; fst != nil; fst = fst.next {
		for snd := fst.next; snd != nil; snd = snd.next {
			if strings.Compare(fst.val, snd.val) == 1 {
				fst.val, snd.val = snd.val, fst.val
			}
		}
	}
	return nil
}

func childSubroutine(l *list, toChild <-chan struct{}, fromChild chan<- struct{}, wg *sync.WaitGroup) {
	defer wg.Done()
	if l == nil {
		return
	}
	for {
		time.Sleep(5 * time.Second)
		select {
		case <-toChild:
			return
		default:
			if err := l.bubbleSort(); err != nil {
				fmt.Fprintf(os.Stderr, "ERROR: %v", err)
				close(fromChild)
				return
			}
		}
	}
}

func main() {
	lst := &list{}
	if err := lst.init(); err != nil {
		fmt.Fprintf(os.Stderr, "ERROR: %v", err)
		return
	}

	toChild := make(chan struct{})
	fromChild := make(chan struct{})
	wg := &sync.WaitGroup{}

	wg.Add(1)
	go childSubroutine(lst, toChild, fromChild, wg)

	scanner := bufio.NewScanner(os.Stdin)
	for {
		select {
		case <-fromChild:
			wg.Wait()
			return
		default:
			fmt.Print("Input some string: ")
			if !scanner.Scan() {
				close(toChild)
				wg.Wait()
				return
			}
			input := scanner.Text()
			if input == "" {
				if err := lst.printList(); err != nil {
					close(toChild)
					wg.Wait()
					fmt.Fprintf(os.Stderr, "ERROR: %v", err)
					return
				}
			} else {
				chunks := sepString(input)
				for _, str := range chunks {
					if err := lst.add(str); err != nil {
						close(toChild)
						wg.Wait()
						fmt.Fprintf(os.Stderr, "ERROR: %v", err)
						return
					}
				}
			}
		}
	}
}

func sepString(input string) []string {
	runes := []rune(input)
	var chunks []string
	limit := 80

	for i := 0; i < len(runes); i += limit {
		end := i + limit

		if end > len(runes) {
			end = len(runes)
		}

		chunks = append(chunks, string(runes[i:end]))
	}

	return chunks
}
