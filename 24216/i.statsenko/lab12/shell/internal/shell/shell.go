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
}

func NewShell() *Shell {
	pmpt := prompt.Prompt{fmt.Sprintf("[%s]~ ", os.Args[0])}
	procManager := process.NewProcessManager(pmpt)
	pars := parser.NewParser(pmpt)
	return &Shell{pmpt: pmpt, procManager: procManager, parser: pars}

}

func (shell *Shell) Run() {
	var err error
	defer shell.procManager.KillAll()

	for shell.commands, err = shell.parser.ParseLine(); ; shell.commands, err = shell.parser.ParseLine() {

		if errors.Is(err, io.EOF) {
			fmt.Println("exit")
			return
		}
		if err != nil {
			fmt.Println(err)
			continue
		}

		err = shell.launchAll()
		if err != nil {
			fmt.Println(err)
			if errors.Is(err, execute.ErrExit) {
				return
			}
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
