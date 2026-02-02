package main

import (
	"fmt"
	"sync"
)

func printer(strs []string) {
	for _, str := range strs {
		fmt.Println(str)
	}
}

func main() {
	data := [][]string{
		{"1 Thread, 1 string", "1 Thread, 2 string", "1 Thread, 3 string"},
		{"2 Thread, 1 string", "2 Thread, 2 string", "2 Thread, 3 string"},
		{"3 Thread, 1 string", "3 Thread, 2 string", "3 Thread, 3 string"},
		{"4 Thread, 1 string", "4 Thread, 2 string", "4 Thread, 3 string"},
	}

	var wg sync.WaitGroup
	for _, strs := range data {
		wg.Add(1)
		go func(s []string) {
			defer wg.Done()
			printer(s)
		}(strs)
	}
	wg.Wait()
	fmt.Println("Finished")
}
