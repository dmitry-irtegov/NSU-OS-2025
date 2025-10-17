#pragma once

#include "parse/parser.h"
#include "parse/user_input.h"

typedef enum parsing_return_code {
	PARSING_UNFINISHED,
	PARSING_EOL,
	PARSING_ERROR
} parsing_return_code;

parsing_return_code parse_line(char *line, user_input *inp);

