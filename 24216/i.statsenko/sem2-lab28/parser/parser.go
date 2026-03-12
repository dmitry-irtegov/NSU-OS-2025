package parser

type Parser struct {
	header     bool
	buf        []byte
	countGiven int
	width      int
}

const maxCountGiven = 25

func NewParser() *Parser {
	return &Parser{
		buf:   make([]byte, 0, 4096),
		width: 80,
	}
}

func (p *Parser) SetWidth(w int) {
	if w > 0 {
		p.width = w
	}
}

func (p *Parser) Parse(data []byte) ([]string, error) {
	p.buf = append(p.buf, data...)
	p.skipHeader()
	lines := make([]string, 0, 16)
	start := 0
	for i, b := range p.buf {
		if p.countGiven >= maxCountGiven {
			break
		}
		if b == '\n' {
			end := i
			if end > start && p.buf[end-1] == '\r' {
				end--
			}
			lineLen := end - start
			lines = append(lines, string(p.buf[start:end]))
			start = i + 1
			rows := 1
			if p.width > 0 && lineLen > p.width {
				rows = (lineLen + p.width - 1) / p.width
			}
			p.countGiven += rows
		}
	}
	n := copy(p.buf, p.buf[start:])
	p.buf = p.buf[:n]
	return lines, nil
}

func (p *Parser) ResetCountGiven() {
	p.countGiven = 0
}

func (p *Parser) IsMaxCountGiven() bool {
	return p.countGiven >= maxCountGiven
}

func (p *Parser) Len() int {
	return len(p.buf)
}

func (p *Parser) Flush() {
	if len(p.buf) > 0 && p.buf[len(p.buf)-1] != '\n' {
		p.buf = append(p.buf, '\n')
	}
}

func (p *Parser) skipHeader() {
	if p.header {
		return
	}
	byteHeader := []byte{'\r', '\n', '\r', '\n'}
	for i := 0; i < len(p.buf)-3; i++ {
		for j := 0; j < len(byteHeader); j++ {
			if p.buf[i+j] != byteHeader[j] {
				break
			}
			if j == len(byteHeader)-1 {
				n := copy(p.buf, p.buf[i+len(byteHeader):])
				p.buf = p.buf[:n]
				p.header = true
				return
			}
		}
	}
}
