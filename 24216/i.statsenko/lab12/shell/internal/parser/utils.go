package parser

import (
	"bufio"
	"os"
	"unicode"
)

func (parser *Parser) blankskip() {
	for (parser.index < parser.length) && (unicode.IsSpace(parser.commandLine[parser.index])) {
		parser.index++
	}
}

func (parser *Parser) strpbrk() string {
	name := make([]rune, 0, 32)
	for ; parser.index < parser.length; parser.index++ {
		for _, elem := range delim {
			if elem == parser.commandLine[parser.index] { // тоже кака
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
	quotes := quotations{}
	for {
		data, err := reader.ReadBytes('\n')
		if err != nil {
			return err
		}
		partLine := quotes.findQuote([]rune(string(data)))
		lnPart := len(partLine)
		if quotes.isQuote() {
			partLine = partLine[:lnPart-1]
			parser.commandLine = append(parser.commandLine, partLine...)
			parser.pmpt.PrintContinueLine()
			continue
		}
		if (lnPart == 1) && (partLine[0] == '\n') {
			parser.index = 0
			parser.length = len(parser.commandLine)
			return nil
		}
		if partLine[lnPart-2] != '\\' {
			partLine = partLine[:lnPart-1]
			parser.commandLine = append(parser.commandLine, partLine...)
			break
		}
		partLine = partLine[:lnPart-2]
		parser.commandLine = append(parser.commandLine, partLine...)
		parser.pmpt.PrintContinueLine()
	}
	parser.index = 0
	parser.length = len(parser.commandLine)
	return nil
}
