package shell

import (
	"fmt"
	"os"
	"shell/internal/commandExec"
	"shell/internal/prompt"
	"shell/internal/strWork"
)

/*  cmdflag's  */
const OUTPIP int = 01
const INPIP int = 02

func Shell() {
	var cmds commandExec.CmdRequest
	var i int
	var ncmds int
	pmpt := prompt.Prompt{fmt.Sprintf("[%s]~ ", os.Args[0])}
	for line := strWork.Promptline(pmpt); line != nil; line = strWork.Promptline(pmpt) {
		if ncmds = cmds.Parseline(line, &ncmds); ncmds <= 0 {
			continue /* read next line */
		}

		for i = 0; i < ncmds; i++ {
			cmds.Cmds[i].Fork(pmpt)
			cmds.Cmds[i] = commandExec.Command{}
		}
		ncmds = 0
		cmds.Cmds = cmds.Cmds[:0]
	} /* close while */
}
