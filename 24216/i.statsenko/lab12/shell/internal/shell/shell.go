package shell

import (
	"fmt"
	"os"
	"shell/internal/execute"
	"shell/internal/process"
	"shell/internal/prompt"
	"shell/internal/utils"
)

type Shell struct {
	commands    execute.CmdRequest
	pmpt        prompt.Prompt
	procManager *process.ProcessManager
}

func NewShell() *Shell {
	pmpt := prompt.Prompt{fmt.Sprintf("[%s]~ ", os.Args[0])}
	procManager := process.NewProcessManager(pmpt)
	return &Shell{pmpt: pmpt, procManager: procManager}

}

func (shell *Shell) Run() {
	var ncmds, pgid int
	var err error
	defer shell.procManager.KillAll()

	for line := utils.Promptline(shell.pmpt); line != nil; line = utils.Promptline(shell.pmpt) {
		if ncmds, err = shell.commands.Parseline(line); err != nil {
			fmt.Println(err)
			continue /* read next line */
		}

		pgid = 0

		for i := 0; i < ncmds; i++ {
			if shell.commands.Cmds[i].Pipe {
				r, w, err := os.Pipe()
				if err != nil {
					fmt.Println(err)
					return
				}
				shell.commands.Cmds[i].OutPipe = w
				shell.commands.Cmds[i+1].InPipe = r
			}
			pgid, _ = shell.commands.Cmds[i].Run(shell.procManager)
			fmt.Println("запустился процесс: ", pgid)
		}
		shell.commands.Ncmds = 0
		shell.commands.Cmds = shell.commands.Cmds[:0]
	}
}
