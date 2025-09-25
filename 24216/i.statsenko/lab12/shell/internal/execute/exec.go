package execute

import (
	"fmt"
	"os"
	"os/exec"
	"shell/internal/process"
	"syscall"
)

func safeClose(file *os.File) {
	err := file.Close()
	if err != nil {
		fmt.Println("Error closing file:", err)
	}
}

func (command *Command) Run(pm *process.ProcessManager) (int, bool) {
	stdin := os.Stdin
	stdout := os.Stdout
	stderr := os.Stderr
	var inPipeLine bool

	binaryPath, err := exec.LookPath(command.Cmdargs[0])
	if err != nil {
		fmt.Println(command.Cmdargs[0], " not found in $PATH")
		return 0, false
	}

	if command.Infile != "" {
		stdin, err = os.OpenFile(command.Infile, os.O_RDONLY, 0)
		if err != nil {
			fmt.Println(err)
			return 0, false
		}
	}

	if command.Outfile != "" {
		stdout, err = os.OpenFile(command.Outfile, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
		if err != nil {
			fmt.Println(err)
			return 0, false
		}
	}

	if command.Appfile != "" {
		stdout, err = os.OpenFile(command.Appfile, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
		if err != nil {
			fmt.Println(err)
			return 0, false
		}
	}

	if command.InPipe != nil {
		stdin = command.InPipe
		inPipeLine = true
	}

	if command.OutPipe != nil {
		stdout = command.OutPipe
		inPipeLine = true
	}

	pid, err := syscall.ForkExec(binaryPath, command.Cmdargs, &syscall.ProcAttr{
		Dir:   "",
		Files: []uintptr{stdin.Fd(), stdout.Fd(), stderr.Fd()},
		Sys: &syscall.SysProcAttr{
			Setpgid: true,
		},
	})

	if stdin != os.Stdin {
		safeClose(stdin)
	}
	if stdout != os.Stdout {
		safeClose(stdout)
	}

	if err != nil {
		fmt.Println(err)
		return 0, false
	}
	if command.Bkgrnd {
		fmt.Println("PID: ", pid)
		go pm.Wait(pid, true)
		fmt.Println("Done ", pid)
		pm.Prompt.PrintPrompt()
	} else {
		pm.Wait(pid, false)
	}
	return pid, inPipeLine
}
