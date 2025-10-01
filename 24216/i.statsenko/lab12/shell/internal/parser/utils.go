package parser

import (
	"bufio"
	"os"
	"unicode"
)

func (parser *Parser) blankskip() {
	for (parser.index < parser.length) && (unicode.IsSpace(parser.commandLine[parser.index])) { // кака
		parser.index++
	}
}

func (parser *Parser) strpbrk() string {
	name := make([]rune, 0, 32)
	for ; parser.index < parser.length; parser.index++ {
		for _, elem := range delim {
			if elem == parser.commandLine[parser.index] {
				return string(name)
			}
		}
		name = append(name, parser.commandLine[parser.index])
	}
	return string(name)
}

func (parser *Parser) promptline() error {
	parser.pmpt.PrintPrompt()
	parser.commandLine = parser.commandLine[:0]
	reader := bufio.NewReader(os.Stdin)
	for {
		partLine, err := reader.ReadBytes('\n')
		if err != nil {
			return err
		}
		lnPart := len(partLine)
		if (lnPart == 1) && (partLine[0] == '\n') {
			parser.index = 0
			parser.length = len(parser.commandLine)
			return nil
		}
		if partLine[lnPart-2] != '\\' {
			partLine[lnPart-1] = ' '
			parser.commandLine = append(parser.commandLine, []rune(string(partLine))...)
			break
		}
		partLine[lnPart-1] = ' '
		partLine[lnPart-2] = ' '
		parser.commandLine = append(parser.commandLine, []rune(string(partLine))...)
	}
	parser.index = 0
	parser.length = len(parser.commandLine)
	return nil
}
