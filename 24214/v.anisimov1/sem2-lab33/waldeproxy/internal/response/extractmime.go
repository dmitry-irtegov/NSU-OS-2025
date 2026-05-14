package response

import "strings"

func ExtractMIME(response []byte) string {
	stringResp := string(response)
	lines := strings.Split(stringResp, "\r\n")
	for _, line := range lines {
		lowline := strings.ToLower(line)
		if strings.Contains(lowline, "content-type:") {
			tokens := strings.SplitN(lowline, " ", 2)

			if len(tokens) != 2 {
				return ""
			}

			return strings.Trim(tokens[1], " \r\n")
		}
	}
	return ""
}
