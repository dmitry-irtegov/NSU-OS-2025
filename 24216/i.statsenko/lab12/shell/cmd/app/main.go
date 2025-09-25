package main

import (
	"os/signal"
	"shell/internal/shell"
	"syscall"
)

func main() {
	signal.Ignore(syscall.SIGTSTP)
	sh := shell.NewShell()
	sh.Run()
}
