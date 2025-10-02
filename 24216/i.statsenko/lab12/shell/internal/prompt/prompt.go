package prompt

import "fmt"

type Prompt struct {
	Prmpt string
}

func (p *Prompt) PrintPrompt() {
	fmt.Print(p.Prmpt)
}
