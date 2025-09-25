package execute

import (
	"fmt"
	"shell/internal/utils"
)

func (cmds *CmdRequest) Parseline(line []byte) (int, error) {
	var argCount, returnValue int
	var appendFlag bool
	const delim string = " \t|&<>;\n"

	lnLine := len(line)
	cmds.Cmds = append(cmds.Cmds, Command{})
	for strIndex := 0; strIndex < lnLine; {
		strIndex = utils.Blankskip(line, strIndex, lnLine)
		if strIndex >= lnLine {
			break
		}

		switch line[strIndex] {
		case '&':
			cmds.Cmds[cmds.Ncmds].Bkgrnd = true
			fallthrough
		case ';':
			strIndex++
			cmds.Ncmds++
			argCount = 0
			cmds.Cmds = append(cmds.Cmds, Command{})
		case '>':
			if (strIndex+1 < lnLine) && (line[strIndex+1] == '>') {
				appendFlag = true
				strIndex++
			}
			strIndex++
			strIndex = utils.Blankskip(line, strIndex, lnLine)
			if strIndex >= lnLine {
				return -1, fmt.Errorf("syntax error")
			}
			if appendFlag {
				cmds.Cmds[cmds.Ncmds].Appfile, strIndex = utils.Strpbrk(line, strIndex, lnLine, delim)
			} else {
				cmds.Cmds[cmds.Ncmds].Outfile, strIndex = utils.Strpbrk(line, strIndex, lnLine, delim)
			}
			appendFlag = false
		case '<':
			strIndex++
			strIndex = utils.Blankskip(line, strIndex, lnLine)
			if strIndex >= lnLine {
				fmt.Println("syntax error")
				return -1, fmt.Errorf("syntax error")
			}
			cmds.Cmds[cmds.Ncmds].Infile, strIndex = utils.Strpbrk(line, strIndex, lnLine, delim)
		case '|':
			cmds.Cmds[cmds.Ncmds].Pipe = true
			strIndex++
			cmds.Ncmds++
			argCount = 0
			cmds.Cmds = append(cmds.Cmds, Command{})
		default:
			if argCount == 0 {
				returnValue = cmds.Ncmds + 1
			}
			var arg string
			arg, strIndex = utils.Strpbrk(line, strIndex, lnLine, delim)
			cmds.Cmds[cmds.Ncmds].Cmdargs = append(cmds.Cmds[cmds.Ncmds].Cmdargs, arg)
			argCount++
		}
	}
	if cmds.Cmds[cmds.Ncmds].Pipe {
		fmt.Println("syntax error")
		return -1, fmt.Errorf("syntax error")
	}

	return returnValue, nil
}
