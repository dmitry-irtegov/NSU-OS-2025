package parser

import (
	"fmt"
	"shell/internal/execute"
	"shell/internal/prompt"
)

type Parser struct {
	commandLine []rune
	index       int
	length      int
	pmpt        prompt.Prompt
}

func NewParser(pmpt prompt.Prompt) *Parser {
	return &Parser{
		commandLine: make([]rune, 0, 1024),
		index:       0,
		length:      0,
		pmpt:        pmpt,
	}
}

func (parser *Parser) ParseLine() (execute.CmdRequest, error) {
	var argCount, returnValue int
	var appendFlag bool
	cmds := execute.CmdRequest{
		Ncmds: 0,
		Cmds:  make([]execute.Command, 0),
	}

	err := parser.promptline()
	if err != nil {
		return cmds, err
	}

	cmds.Cmds = append(cmds.Cmds, execute.Command{})
	for parser.index < parser.length {
		parser.blankskip()
		if parser.index >= parser.length {
			break
		}

		switch parser.commandLine[parser.index] {
		case '&':
			cmds.Cmds[cmds.Ncmds].Bkgrnd = true
			fallthrough
		case ';':
			parser.index++
			cmds.Ncmds++
			argCount = 0
			cmds.Cmds = append(cmds.Cmds, execute.Command{})
		case '>':
			if (parser.index+1 < parser.length) && (parser.commandLine[parser.index+1] == '>') {
				appendFlag = true
				parser.index++
			}
			parser.index++
			parser.blankskip()
			if parser.index >= parser.length {
				return cmds, fmt.Errorf("syntax error")
			}
			if appendFlag {
				cmds.Cmds[cmds.Ncmds].Appfile = parser.strpbrk()
			} else {
				cmds.Cmds[cmds.Ncmds].Outfile = parser.strpbrk()
			}
			appendFlag = false
		case '<':
			parser.index++
			parser.blankskip()
			if parser.index >= parser.length {
				fmt.Println("syntax error")
				return cmds, fmt.Errorf("syntax error")
			}
			cmds.Cmds[cmds.Ncmds].Infile = parser.strpbrk()
		case '|':
			cmds.Cmds[cmds.Ncmds].Pipe = true
			parser.index++
			cmds.Ncmds++
			argCount = 0
			cmds.Cmds = append(cmds.Cmds, execute.Command{})
		default:
			if argCount == 0 {
				returnValue = cmds.Ncmds + 1
			}
			var arg string
			arg = parser.strpbrk()
			cmds.Cmds[cmds.Ncmds].Cmdargs = append(cmds.Cmds[cmds.Ncmds].Cmdargs, arg)
			argCount++
		}
	}
	if cmds.Cmds[cmds.Ncmds].Pipe {
		fmt.Println("syntax error")
		return cmds, fmt.Errorf("syntax error")
	}
	cmds.Ncmds = returnValue
	return cmds, nil
}
