package client

import (
	"strconv"
	"strings"
)

func getExpectedBodySize(headers []byte) int {
	headerStr := string(headers)

	lines := strings.Split(headerStr, "\r\n")

	for _, line := range lines {
		lowerLine := strings.ToLower(line)

		if strings.HasPrefix(lowerLine, "content-length:") {
			parts := strings.SplitN(line, ":", 2)
			if len(parts) == 2 {
				valStr := strings.TrimSpace(parts[1])

				size, err := strconv.Atoi(valStr)
				if err == nil {
					return size
				}
			}
		}
	}

	return 0
}
