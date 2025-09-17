package commandExec

import (
	"fmt"
	"shell/internal/strWork"
)

type Command struct {
	Cmdargs                  []string
	Infile, Outfile, Appfile string
	Cmdflag                  byte
	Bkgrnd                   byte
}

type CmdRequest struct {
	Cmds []Command
}

func (cmds *CmdRequest) Parseline(line []byte, ncmds *int) int {
	var nargs int
	var aflg byte
	const delim string = " \t|&<>;\n"
	var rval int

	lnLine := len(line)
	cmds.Cmds = append(cmds.Cmds, Command{})
	for strIndex := 0; strIndex < lnLine; {
		strIndex = strWork.Blankskip(line, strIndex, lnLine)
		if strIndex >= lnLine {
			break
		}

		/*  handle <, >, |, &, and ;  */
		switch line[strIndex] {
		case '&':
			cmds.Cmds[*ncmds].Bkgrnd = 1
			fallthrough
		case ';':
			strIndex++
			(*ncmds)++
			nargs = 0
			cmds.Cmds = append(cmds.Cmds, Command{})
		case '>':
			if (strIndex+1 < lnLine) && (line[strIndex+1] == '>') {
				aflg = 1
				strIndex++
			}
			strIndex++
			strIndex = strWork.Blankskip(line, strIndex, lnLine)
			if strIndex >= lnLine {
				fmt.Println("syntax error")
				return -1
			}
			if aflg == 1 {
				cmds.Cmds[*ncmds].Appfile, strIndex = strWork.Strpbrk(line, strIndex, lnLine, delim)
			} else {
				cmds.Cmds[*ncmds].Outfile, strIndex = strWork.Strpbrk(line, strIndex, lnLine, delim)
			}
			aflg = 0
		case '<':
			strIndex++
			strIndex = strWork.Blankskip(line, strIndex, lnLine)
			if strIndex >= lnLine {
				fmt.Println("syntax error")
				return -1
			}
			cmds.Cmds[*ncmds].Infile, strIndex = strWork.Strpbrk(line, strIndex, lnLine, delim)
		case '|':
			if nargs == 0 {
				fmt.Println("syntax error")
				return -1
			} // пока не работает
			fmt.Println("конвееров пока неть(")
			/*cmds[ncmds++].cmdflag |= OUTPIP;
			  cmds[ncmds].cmdflag |= INPIP;
			  *s++ = '\0';
			  nargs = 0;
			  break;*/
		default:
			/*  a command argument  */
			if nargs == 0 {
				rval = *ncmds + 1
			}
			var arg string
			arg, strIndex = strWork.Strpbrk(line, strIndex, lnLine, delim)
			cmds.Cmds[*ncmds].Cmdargs = append(cmds.Cmds[*ncmds].Cmdargs, arg)
			nargs++
		} /*  close switch  */
	} /* close while  */

	/*  error check  */

	/*
	 *  The only errors that will be checked for are
	 *  no command on the right side of a pipe
	 *  no command to the left of a pipe is checked above
	 */
	//if (cmds[*ncmds - 1].cmdflag & OUTPIP) {
	//    if (nargs == 0) {
	//        fprintf(stderr, "syntax error\n");
	//        return(-1);
	//    }
	//}
	return rval
}
