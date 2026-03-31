package main

import (
	"bufio"
	"fmt"
	"os"
	"sync"
	"time"
)

func main() {
	list := InitList()
	var wg sync.WaitGroup
	scanner := bufio.NewScanner(os.Stdin)

	for amount := 0; amount < 100 && scanner.Scan(); amount += 1 {
		text := scanner.Text()

		wg.Add(1)

		go func(s string) {
			defer wg.Done()

			time.Sleep(time.Duration(len(s)) * time.Millisecond * 1500)

			list.pushBack(s)
		}(text)
	}

	if err := scanner.Err(); err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
	}

	wg.Wait()

	fmt.Println("\nResult:")
	list.printList()
}
