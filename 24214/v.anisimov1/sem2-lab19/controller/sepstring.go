package controller

func sepString(input string) []string {
	runes := []rune(input)
	var chunks []string
	limit := 80

	for i := 0; i < len(runes); i += limit {
		end := i + limit

		if end > len(runes) {
			end = len(runes)
		}

		chunks = append(chunks, string(runes[i:end]))
	}

	return chunks
}
