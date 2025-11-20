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
		Cmds:  make([]execute.Command, 0, 4),
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
			if cmds.Cmds[cmds.Ncmds].Cmdargs == nil {
				return cmds, fmt.Errorf("%w: отсутствует команда перед ; или &", ErrSyntax)
			}
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
				return cmds, fmt.Errorf("%w: отсутствует выходной файл", ErrSyntax)
			}
			if appendFlag {
				cmds.Cmds[cmds.Ncmds].Appfile = parser.strpbrk()
				if cmds.Cmds[cmds.Ncmds].Appfile == "" {
					return cmds, fmt.Errorf("%w: отсутствует выходной файл", ErrSyntax)
				}
			} else {
				cmds.Cmds[cmds.Ncmds].Outfile = parser.strpbrk()
				if cmds.Cmds[cmds.Ncmds].Outfile == "" {
					return cmds, fmt.Errorf("%w: отсутствует выходной файл", ErrSyntax)
				}
			}
			appendFlag = false
		case '<':
			parser.index++
			parser.blankskip()
			if parser.index >= parser.length {
				return cmds, fmt.Errorf("%w: отсутствует входной файл", ErrSyntax)
			}
			cmds.Cmds[cmds.Ncmds].Infile = parser.strpbrk()
			if cmds.Cmds[cmds.Ncmds].Infile == "" {
				return cmds, fmt.Errorf("%w: отсутствует входной файл", ErrSyntax)
			}
		case '|':
			if cmds.Cmds[cmds.Ncmds].Cmdargs == nil {
				return cmds, fmt.Errorf("%w: отсутствует команда перед конвеером", ErrSyntax)
			}
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
		return cmds, fmt.Errorf("%w: отсутствует последняя команда в конвеере", ErrSyntax)
	}
	cmds.Ncmds = returnValue
	return cmds, nil
}
