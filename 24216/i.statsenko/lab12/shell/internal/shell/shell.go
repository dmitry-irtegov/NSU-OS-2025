package shell

import (
	"fmt"
	"os"
	"shell/internal/commandExec"
)

/*  cmdflag's  */
const OUTPIP int = 01
const INPIP int = 02

func Shell() {
	var cmds commandExec.CmdRequest
	var i int
	var ncmds int
	prompt := fmt.Sprintf("[%s]~ ", os.Args[0])
	for line := commandExec.Promptline(prompt); line != nil; line = commandExec.Promptline(prompt) {
		if ncmds = cmds.Parseline(line, &ncmds); ncmds <= 0 {
			continue /* read next line */
		}

		for i = 0; i < ncmds; i++ {
			cmds.Cmds[i].Fork()
			cmds.Cmds[i] = commandExec.Command{}
		}
		ncmds = 0
		cmds.Cmds = cmds.Cmds[:0]
	} /* close while */
}
