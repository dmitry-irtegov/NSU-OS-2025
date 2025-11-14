package parser

import (
	"bufio"
	"os"
	"slices"
	"unicode"
)

func (parser *Parser) blankskip() {
	for (parser.index < parser.length) && (unicode.IsSpace(parser.commandLine[parser.index])) {
		parser.index++
	}
}

func (parser *Parser) strpbrk() string {
	currentQuote := rune(0)
	withoutCondition := false
	name := make([]rune, 0, 32)
	for ; parser.index < parser.length; parser.index++ {
		char := parser.commandLine[parser.index]
		switch {
		case withoutCondition:
			withoutCondition = false
			name = append(name, char)
		case (char == '\\') && (currentQuote != '\''):
			withoutCondition = true
		case currentQuote == char:
			currentQuote = rune(0)
		case currentQuote == rune(0) && (char == '\'' || char == '"'):
			currentQuote = char
		case currentQuote == rune(0) && slices.Contains([]rune(delim), char):
			return string(name)
		default:
			name = append(name, char)
		}
	}
	return string(name)
}

func (parser *Parser) promptline() error {
	parser.pmpt.PrintPrompt()
	parser.commandLine = parser.commandLine[:0]
	reader := bufio.NewReader(os.Stdin)
	quotes := delimes{}
	for {
		data, err := reader.ReadBytes('\n')
		if err != nil {
			return err
		}
		partLine := []rune(string(data))
		quotes.findQuote(partLine)
		lnPart := len(partLine)
		if quotes.waitNewLine() {
			if quotes.firstQuote == rune(0) {
				partLine = partLine[:lnPart-1]
			}
			parser.commandLine = append(parser.commandLine, partLine...)
			parser.pmpt.PrintContinueLine()
			continue
		}
		if (lnPart == 1) && (partLine[0] == '\n') {
			parser.index = 0
			parser.length = len(parser.commandLine)
			return nil
		}
		countBack := 0
		for i := lnPart - 2; i >= 0; i-- {
			if partLine[i] != '\\' {
				break
			}
			countBack++
		}
		if countBack%2 == 0 {
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
