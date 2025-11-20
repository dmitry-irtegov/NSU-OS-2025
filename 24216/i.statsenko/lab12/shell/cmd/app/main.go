package main

import (
	"fmt"
	"log"
	"shell/internal/shell"
)

func main() {
	errChan := make(chan error, 1)
	sh := shell.NewShell(errChan)
	go sh.Run()
	err := <-errChan
	if err != nil {
		log.Fatal(err)
	}
	fmt.Println("exit")
}
