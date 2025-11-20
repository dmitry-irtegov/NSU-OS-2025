package parser

type delimes struct {
	firstQuote       rune
	withoutCondition bool
}

func (del *delimes) findQuote(line []rune) {
	for _, elem := range line {
		if del.withoutCondition {
			del.withoutCondition = false
			continue
		}
		switch elem {
		case '\\':
			if del.firstQuote != '\'' {
				del.withoutCondition = true
			}
		case '\'':
			switch del.firstQuote {
			case rune(0):
				del.firstQuote = '\''
			case '\'':
				del.firstQuote = rune(0)
			}
		case '"':
			switch del.firstQuote {
			case rune(0):
				del.firstQuote = '"'
			case '"':
				del.firstQuote = rune(0)
			}
		}
	}
}

func (del *delimes) waitNewLine() bool {
	return del.firstQuote != rune(0) || del.withoutCondition
}
