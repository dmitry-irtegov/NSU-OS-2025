package response

import (
	"strconv"
	"strings"
)

func ResponseValidCheck(response []byte) bool {
	stringResp := string(response)
	lines := strings.Split(stringResp, "\r\n")
	if len(lines) < 1 {
		return false
	}
	startLine := lines[0]
	tokens := strings.Split(startLine, " ")
	if len(tokens) < 3 {
		return false
	}

	if scode, err := strconv.Atoi(tokens[1]); err != nil {
		return false
	} else {
		return scode == 200
	}
}
