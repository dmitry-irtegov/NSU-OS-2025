package main

import (
	"bufio"
	"fmt"
	"lab17/list"
	"os"
	"strings"
	"time"
)

const durationSort = 5 * time.Second

func main() {
	linkList := list.NewLinkedList()
	go linkList.Sort(durationSort)
	reader := bufio.NewReader(os.Stdin)
	for {
		line, err := reader.ReadString('\n')
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		line = strings.TrimRight(line, "\n")

		if line == "" {
			linkList.Print()
			continue
		}
		linkList.AddToEnd(line)
	}
}
