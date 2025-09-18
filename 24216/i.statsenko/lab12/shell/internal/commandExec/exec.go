package commandExec

import (
	"fmt"
	"os"
	"os/exec"
	"shell/internal/prompt"
	"syscall"
)

func safeClose(file *os.File) {
	err := file.Close()
	if err != nil {
		fmt.Println("Error closing file:", err)
	}
}

func (command *Command) Fork(pmpt prompt.Prompt) {
	stdin := os.Stdin
	stdout := os.Stdout
	stderr := os.Stderr

	binnaryPath, err := exec.LookPath(command.Cmdargs[0])
	if err != nil {
		fmt.Println(command.Cmdargs[0], " not found in $PATH")
		return
	}

	if command.Infile != "" {
		stdin, err = os.OpenFile(command.Infile, os.O_RDONLY, 0)
		if err != nil {
			fmt.Println(err)
			return
		}
		defer safeClose(stdin)
	}

	if command.Outfile != "" {
		stdout, err = os.OpenFile(command.Outfile, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
		if err != nil {
			fmt.Println(err)
			return
		}
		defer safeClose(stdout)
	}

	if command.Appfile != "" {
		stdout, err = os.OpenFile(command.Appfile, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
		if err != nil {
			fmt.Println(err)
			return
		}
		defer safeClose(stdout)
	}

	pid, err := syscall.ForkExec(binnaryPath, command.Cmdargs, &syscall.ProcAttr{
		Dir:   "",
		Files: []uintptr{stdin.Fd(), stdout.Fd(), stderr.Fd()},
	})
	if err != nil {
		fmt.Println(err)
		return
	}
	if command.Bkgrnd == 1 {
		fmt.Println("PID: ", pid)
		go func(pt prompt.Prompt) {
			var ws syscall.WaitStatus
			_, err := syscall.Wait4(pid, &ws, 0, nil)
			if err != nil {
				fmt.Println("Error waiting process", err)
				return
			}
			fmt.Println("Done ", pid)
			pt.PrintPrompt()
		}(pmpt)
	} else {
		var ws syscall.WaitStatus
		_, err := syscall.Wait4(pid, &ws, 0, nil)
		if err != nil {
			fmt.Println("Error waiting process", err)
			return
		}
	}
}
