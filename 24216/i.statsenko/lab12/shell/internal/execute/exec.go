package execute

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"shell/internal/process"
	"strconv"
	"strings"
	"syscall"
)

type Pgid = int

func (command *Command) Run(pm *process.Manager, pidGroup Pgid) (Pgid, error) {
	isShellCmd, err := command.shellCommands(pm)
	if err != nil {
		return 0, err
	}
	if isShellCmd {
		return 0, ErrShellCmd
	}
	stdin := os.Stdin
	stdout := os.Stdout
	stderr := os.Stderr

	binaryPath, err := exec.LookPath(command.Cmdargs[0])
	if err != nil {
		return 0, fmt.Errorf("%s not found in $PATH", command.Cmdargs[0])
	}

	switch {
	case command.Infile != "":
		stdin, err = os.OpenFile(command.Infile, os.O_RDONLY, 0)
		if err != nil {
			return 0, err
		}
	case command.Bkgrnd:
		stdin, err = os.OpenFile(os.DevNull, os.O_RDONLY, 0)
		if err != nil {
			return 0, err
		}
	}

	if command.Outfile != "" {
		stdout, err = os.OpenFile(command.Outfile, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
		if err != nil {
			return 0, err
		}
	}

	if command.Appfile != "" {
		stdout, err = os.OpenFile(command.Appfile, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
		if err != nil {
			return 0, err
		}
	}

	if command.InPipe != nil {
		stdin = command.InPipe
	}

	if command.OutPipe != nil {
		stdout = command.OutPipe
	}
	if stdin != os.Stdin {
		defer func() {
			_ = stdin.Close()
		}()
	}
	if stdout != os.Stdout {
		defer func() {
			_ = stdout.Close()
		}()
	}
	var pid Pgid
	attr := syscall.SysProcAttr{
		Setpgid: true,
	}
	if pidGroup != 0 {
		attr.Pgid = pidGroup
	}
	pid, err = syscall.ForkExec(binaryPath, command.Cmdargs, &syscall.ProcAttr{
		Dir:   "",
		Files: []uintptr{stdin.Fd(), stdout.Fd(), stderr.Fd()},
		Sys:   &attr,
	})
	if err != nil {
		return 0, err
	}
	return pid, nil
}

func (command *Command) Wait(pm *process.Manager, pid Pgid) {
	status := process.Foreground
	if command.Bkgrnd {
		status = process.Background
	}
	isPipe := command.InPipe != nil
	data := process.ProcessData{
		Status:       status,
		IsPipe:       isPipe,
		TextCommands: strings.Join(command.Cmdargs, " "),
	}
	if command.Bkgrnd {
		go pm.Wait(pid, data)
		pm.Prompt.PrintPrompt()
	} else {
		pm.Wait(pid, data)
	}
}

func (command *Command) shellCommands(pm *process.Manager) (bool, error) {
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
	case "jobs":
		if len(command.Cmdargs) != 1 {
			return true, fmt.Errorf("jobs: неверное количество аргументов")
		}
		jobs := pm.GetJobs()
		for _, job := range jobs {
			fmt.Println(job)
		}
		return true, nil
	case "bg":
		switch len(command.Cmdargs) {
		case 1:
			jobId, err := pm.GetNearestJobID(process.Background)
			if err != nil {
				return true, err
			}
			return true, pm.ToBackground(jobId)
		case 2:
			jobId, err := strconv.Atoi(command.Cmdargs[1])
			if err != nil {
				return true, fmt.Errorf("bg: аргумент не является числом")
			}
			return true, pm.ToBackground(jobId)
		default:
			return true, fmt.Errorf("bg: неверное количество аргументов")
		}
	case "fg":
		switch len(command.Cmdargs) {
		case 1:
			jobId, err := pm.GetNearestJobID(process.Foreground)
			if err != nil {
				return true, err
			}
			return true, pm.ToForeground(jobId)
		case 2:
			jobId, err := strconv.Atoi(command.Cmdargs[1])
			if err != nil {
				return true, fmt.Errorf("fg: аргумент не является числом")
			}
			return true, pm.ToForeground(jobId)
		default:
			return true, fmt.Errorf("fg: неверное количество аргументов")
		}
	}
	return false, nil
}
