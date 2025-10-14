package shell

import (
	"errors"
	"fmt"
	"io"
	"os"
	"shell/internal/execute"
	"shell/internal/parser"
	"shell/internal/process"
	"shell/internal/prompt"
	"syscall"
)

type Shell struct {
	commands    execute.CmdRequest
	pmpt        prompt.Prompt
	procManager *process.Manager
	parser      *parser.Parser
	errChan     chan<- error
}

func NewShell(errChan chan<- error) *Shell {
	pmpt := prompt.Prompt{}
	procManager := process.NewProcessManager(pmpt, errChan)
	pars := parser.NewParser(pmpt)
	return &Shell{pmpt: pmpt, procManager: procManager, parser: pars, errChan: errChan}

}

func (shell *Shell) Run() {
	var err error
	defer shell.procManager.KillAll()

	for shell.commands, err = shell.parser.ParseLine(); ; shell.commands, err = shell.parser.ParseLine() {

		switch {
		case errors.Is(err, io.EOF):
			shell.errChan <- nil
			return
		case errors.Is(err, parser.ErrSyntax):
			fmt.Println(err)
			continue
		case err != nil:
			fmt.Println(err)
			shell.errChan <- err
			return
		}

		err = shell.launchAll()
		if err != nil {
			if errors.Is(err, execute.ErrExit) {
				shell.errChan <- nil
				return
			}
			fmt.Println(err)
		}
	}
}

func (shell *Shell) launchAll() error {
	for i := 0; i < shell.commands.Ncmds; i++ {
		if shell.commands.Cmds[i].Pipe {
			var err error
			i, err = shell.launchPipe(i)
			if err != nil {
				fmt.Println(err)
			}
			continue
		}
		pid, err := shell.commands.Cmds[i].Run(shell.procManager, 0)
		switch {
		case err == nil:
			shell.commands.Cmds[i].Wait(shell.procManager, pid)
		case errors.Is(err, execute.ErrExit):
			return err
		case !errors.Is(err, execute.ErrShellCmd):
			fmt.Println(err)
		}
	}
	shell.commands.Ncmds = 0
	shell.commands.Cmds = shell.commands.Cmds[:0]
	return nil
}

func (shell *Shell) launchPipe(i int) (int, error) {
	newI := i
	for ; newI < shell.commands.Ncmds; newI++ {
		if shell.commands.Cmds[newI].Pipe {
			r, w, err := os.Pipe()
			if err != nil {
				return newI, err
			}
			shell.commands.Cmds[newI].OutPipe = w
			shell.commands.Cmds[newI+1].InPipe = r
		} else {
			break
		}
	}
	pgid, err := shell.commands.Cmds[newI].Run(shell.procManager, 0)
	if err != nil {
		return newI, err
	}
	_ = shell.commands.Cmds[newI].InPipe.Close()
	for j := newI - 1; j >= i; j-- {
		_, err := shell.commands.Cmds[j].Run(shell.procManager, pgid)
		_ = shell.commands.Cmds[j].InPipe.Close()
		_ = shell.commands.Cmds[j].OutPipe.Close()
		if err != nil {
			_ = syscall.Kill(-pgid, syscall.SIGKILL)
			return newI, err
		}
	}
	shell.commands.Cmds[newI].Wait(shell.procManager, -pgid)
	return newI, nil
}
