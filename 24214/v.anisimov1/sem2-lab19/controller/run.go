package controller

import (
	"bufio"
	"fmt"
	"os"
	"sem2-lab19/list"
	"sync"
)

func Run() {
	lst := list.NewList()

	toChild := make(chan struct{})
	fromChild := make(chan struct{})
	wg := &sync.WaitGroup{}

	wg.Add(1)
	go childSubroutine(lst, toChild, wg)

	scanner := bufio.NewScanner(os.Stdin)
	for {
		select {
		case <-fromChild:
			wg.Wait()
			return
		default:
			fmt.Print("Input string: ")
			if !scanner.Scan() {
				close(toChild)
				wg.Wait()
				return
			}
			input := scanner.Text()
			if input == "" {
				lst.PrintList()
			} else {
				chunks := sepString(input)
				for _, str := range chunks {
					lst.Add(str)
				}
			}
		}
	}
}
