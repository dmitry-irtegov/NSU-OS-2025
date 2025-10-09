package parser

type quotations struct {
	firstQuote rune
}

func (quotes *quotations) findQuote(line []rune) []rune {
	newLine := make([]rune, 0, 32)
	for _, elem := range line {
		switch elem {
		case '\'':
			switch quotes.firstQuote {
			case rune(0):
				quotes.firstQuote = '\''
			case '\'':
				quotes.firstQuote = rune(0)
			default:
				newLine = append(newLine, '\'')
			}
		case '"':
			switch quotes.firstQuote {
			case rune(0):
				quotes.firstQuote = '"'
			case '"':
				quotes.firstQuote = rune(0)
			default:
				newLine = append(newLine, '"')
			}
		default:
			newLine = append(newLine, elem)
		}
	}
	return newLine
}

func (quotes *quotations) isQuote() bool {
	return quotes.firstQuote != rune(0)
}
