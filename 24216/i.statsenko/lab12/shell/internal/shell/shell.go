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

		if errors.Is(err, io.EOF) {
			shell.errChan <- nil
			return
		}
		if err != nil {
			fmt.Println(err)
			continue
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
			r, w, err := os.Pipe()
			if err != nil {
				return err
			}
			shell.commands.Cmds[i].OutPipe = w
			shell.commands.Cmds[i+1].InPipe = r
		}
		err := shell.commands.Cmds[i].Run(shell.procManager)
		if err != nil {
			return err
		}
	}
	shell.commands.Ncmds = 0
	shell.commands.Cmds = shell.commands.Cmds[:0]
	return nil
}
