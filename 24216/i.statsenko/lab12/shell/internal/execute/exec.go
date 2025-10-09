package execute

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"shell/internal/process"
	"syscall"
)

func (command *Command) Run(pm *process.Manager) error {
	isShellCmd, err := command.shellCommands()
	if err != nil {
		return err
	}
	if isShellCmd {
		return nil
	}
	stdin := os.Stdin
	stdout := os.Stdout
	stderr := os.Stderr

	binaryPath, err := exec.LookPath(command.Cmdargs[0])
	if err != nil {
		return fmt.Errorf("%s not found in $PATH", command.Cmdargs[0])
	}

	switch {
	case command.Infile != "":
		stdin, err = os.OpenFile(command.Infile, os.O_RDONLY, 0)
		if err != nil {
			return err
		}
	case command.Bkgrnd:
		stdin, err = os.OpenFile(os.DevNull, os.O_RDONLY, 0)
		if err != nil {
			return err
		}
	}

	if command.Outfile != "" {
		stdout, err = os.OpenFile(command.Outfile, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
		if err != nil {
			return err
		}
	}

	if command.Appfile != "" {
		stdout, err = os.OpenFile(command.Appfile, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
		if err != nil {
			return err
		}
	}

	if command.InPipe != nil {
		stdin = command.InPipe
	}

	if command.OutPipe != nil {
		stdout = command.OutPipe
	}
	pid, err := syscall.ForkExec(binaryPath, command.Cmdargs, &syscall.ProcAttr{
		Dir:   "",
		Files: []uintptr{stdin.Fd(), stdout.Fd(), stderr.Fd()},
		Sys: &syscall.SysProcAttr{
			Setpgid: true,
		},
	})
	if err != nil {
		return err
	}
	if stdin != os.Stdin {
		_ = stdin.Close()
	}
	if stdout != os.Stdout {
		_ = stdout.Close()
	}

	data := process.ProcessData{
		Background: command.Bkgrnd,
	}
	if command.Bkgrnd {
		fmt.Println("PID: ", pid)
		go pm.Wait(pid, data)
		fmt.Println("Done ", pid)
		pm.Prompt.PrintPrompt()
	} else {
		pm.Wait(pid, data)
	}
	return nil
}

func (command *Command) shellCommands() (bool, error) {
	switch command.Cmdargs[0] {
	case "cd":
		if len(command.Cmdargs) != 2 {
			return true, fmt.Errorf("cd: неверное количество аргументов")
		}
		curPath, err := os.Getwd()
		if err != nil {
			return true, err
		}
		path := filepath.Clean(filepath.Join(curPath, command.Cmdargs[1]))
		err = os.Chdir(path)
		if err != nil {
			return true, err
		}
		return true, nil
	case "exit":
		return true, ErrExit
	}
	return false, nil
}
