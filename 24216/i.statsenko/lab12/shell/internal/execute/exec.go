package execute

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"shell/internal/process"
	"strconv"
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
	if pidGroup != 0 {
		pid, err = syscall.ForkExec(binaryPath, command.Cmdargs, &syscall.ProcAttr{
			Dir:   "",
			Files: []uintptr{stdin.Fd(), stdout.Fd(), stderr.Fd()},
			Sys: &syscall.SysProcAttr{
				Setpgid: true,
				Pgid:    pidGroup,
			},
		})
	} else {
		pid, err = syscall.ForkExec(binaryPath, command.Cmdargs, &syscall.ProcAttr{
			Dir:   "",
			Files: []uintptr{stdin.Fd(), stdout.Fd(), stderr.Fd()},
			Sys: &syscall.SysProcAttr{
				Setpgid: true,
			},
		})
	}
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
		Status: status,
		IsPipe: isPipe,
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
		if len(command.Cmdargs) != 2 {
			return true, fmt.Errorf("bg: неверное количество аргументов")
		}
		jobId, err := strconv.Atoi(command.Cmdargs[1])
		if err != nil {
			return true, fmt.Errorf("bg: аргумент не является числом")
		}
		err = pm.ToBackground(jobId)
		if err != nil {
			return true, err
		}
		return true, nil
	case "fg":
		if len(command.Cmdargs) != 2 {
			return true, fmt.Errorf("fg: неверное количество аргументов")
		}
		jobId, err := strconv.Atoi(command.Cmdargs[1])
		if err != nil {
			return true, fmt.Errorf("fg: аргумент не является числом")
		}
		err = pm.ToForeground(jobId)
		if err != nil {
			return true, err
		}
		return true, nil
	}
	return false, nil
}
