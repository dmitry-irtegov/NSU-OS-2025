package commandExec

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"unicode"
)

func blankskip(line []byte, start, lnLine int) int {
	for (start < lnLine) && (unicode.IsSpace(rune(line[start]))) {
		start++
	}
	return start
}

func strpbrk(line []byte, start, lnLine int, delim string) (string, int) {
	name := make([]byte, 0, 32)
	for i := start; i < lnLine; i++ {
		for _, elem := range delim {
			if elem == rune(line[i]) {
				return string(name), i
			}
		}
		name = append(name, line[i])
	}
	return string(name), lnLine - 1
}

func Promptline(prompt string) []byte {
	fmt.Print(prompt)
	reader := bufio.NewReader(os.Stdin)
	line := make([]byte, 0, 1024)
	for {
		partLine, err := reader.ReadBytes('\n')
		if (err != nil) && (err != io.EOF) {
			fmt.Println("Error reading line: ", err)
			return line
		}
		lnPart := len(partLine)
		if (lnPart == 1) && (partLine[0] == '\n') {
			return line
		}
		if (err == io.EOF) || (partLine[lnPart-2] != '\\') {
			partLine[lnPart-1] = ' '
			line = append(line, partLine...)
			break
		}
		partLine[lnPart-1] = ' '
		partLine[lnPart-2] = ' '
		line = append(line, partLine...)
	}
	return line
}
