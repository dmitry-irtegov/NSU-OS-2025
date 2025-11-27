package main

import (
	"fmt"
	"os"
	"syscall"
)

const fileName string = "file.txt"

func TryToOpenFile(name string) {
	file, err := os.Open(name)
	if err != nil {
		fmt.Println(err)
		return
	}
	_ = file.Close()
}

func main() {
	fmt.Println("RUID: ", syscall.Getuid(), "EUID: ", syscall.Geteuid())
	TryToOpenFile(fileName)

	err := syscall.Setuid(syscall.Getuid())
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}

	fmt.Println("RUID: ", syscall.Getuid(), "EUID: ", syscall.Geteuid())
	TryToOpenFile(fileName)
}
