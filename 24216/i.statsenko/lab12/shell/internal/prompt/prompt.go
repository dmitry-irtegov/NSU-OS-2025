package prompt

import (
	"fmt"
	"os"
)

type Prompt struct{}

func (p *Prompt) PrintPrompt() {
	path, err := os.Getwd()
	if err != nil {
		fmt.Print("[unknown directory]$ ")
	}
	fmt.Print("[", path, "]$ ")
}

func (p *Prompt) PrintContinueLine() {
	fmt.Print("> ")
}
