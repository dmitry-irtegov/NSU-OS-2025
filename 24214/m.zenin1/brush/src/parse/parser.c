#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "parse/parser.h"
#include "parse/command.h"
#include "parse/user_input.h"
#include "parse/pipeline.h"
#include "ptr_vec.h"
#include "charstream.h"
#include "shell/shell.h"

#define IS_PIPELINE_DELIM(chr) ((chr) == ';' || (chr) == '&')
#define IS_COMMAND_DELIM(chr) ((chr) == '|')
#define IS_META(chr) ((chr) == '<' || (chr) == '>')
#define IS_EOL(chr) ((chr) == '\0')
#define IS_BLANK(chr) (isblank(chr) || (chr) == '\n')

static parsing_return_code parse_pipeline(char **cursor, pipeline *pipl);
static parsing_return_code parse_command(char **cursor, command *cmd);
static parsing_return_code parse_arg(char **cursor, command *cmd);
static parsing_return_code process_meta(char **cursor, pipeline *pipl);
static void skip_blank(char **cursor);

static parsing_return_code get_arg(char **cursor, char **out);
static char* filter_arg(char *start, size_t size);

parsing_return_code parse_line(char *line, user_input *inp){
	char *cursor = line;
	skip_blank(&cursor);

	while (1){
		if (IS_PIPELINE_DELIM(*cursor)){
			fprintf(stderr, "Expected pipeline, got: \"%c\"\n", *cursor);
			return PARSING_ERROR;
		}

		parsing_return_code ret = parse_pipeline(&cursor, user_input_allocate_new_pipeline(inp));

		if (ret == PARSING_EOL || ret == PARSING_ERROR){
			return ret;
		}

		pipeline *last_pipl = (pipeline*) inp->pipelines.arr[inp->pipelines.size - 1];

		if (*cursor == '&'){
			last_pipl->foreground = false;
		}
		// Skip PIPELINE_DELIM
		cursor++;

		skip_blank(&cursor);

		if (IS_EOL(*cursor)){
			return PARSING_EOL;
		}
	}
}

// Assume what *cursor points to the start of pipeline (not blank, not pipeline delim, not command ddelin)
// Move cursor to first pipeline delim or
// if end of string is reached return PARSING_EOL
static parsing_return_code parse_pipeline(char **cursor, pipeline *pipl){
	if (!(**cursor)){
		fprintf(stderr, "Unexpected end of line\n");
		return PARSING_ERROR;
	}
	if (IS_BLANK(**cursor) || IS_COMMAND_DELIM(**cursor) || IS_PIPELINE_DELIM(**cursor)){
		fprintf(stderr, "Syntax error, expected start of pipeline, got: \"%c\"\n", **cursor);
		return PARSING_ERROR;
	}

	char *pipeline_start = *cursor;

	while (1){
		parsing_return_code ret;

		if (IS_META(**cursor)){
			ret = process_meta(cursor, pipl);
		}
		else{
			ret = parse_command(cursor, pipeline_allocate_new_command(pipl));
		}

		if (ret == PARSING_ERROR){
			return ret;
		}
		else if (ret == PARSING_EOL){
			pipeline_set_original_line(pipl, pipeline_start, *cursor - pipeline_start);
			return ret;
		}

		if (IS_PIPELINE_DELIM(**cursor)){
			pipeline_set_original_line(pipl, pipeline_start, *cursor - pipeline_start);
			return PARSING_UNFINISHED;
		}
		else if (IS_COMMAND_DELIM(**cursor)){
			(*cursor)++;
			skip_blank(cursor);

			if (IS_EOL(**cursor)){
				pipeline_set_original_line(pipl, pipeline_start, *cursor - pipeline_start);
				return PARSING_EOL;
			}
			else if (IS_COMMAND_DELIM(**cursor) || IS_PIPELINE_DELIM(**cursor)){
				fprintf(stderr, "Expected start of command or meta-command, got: \"%c\"\n", **cursor);
				return PARSING_ERROR;
			}
		}
		else if (IS_META(**cursor)){
			continue;
		}
		else{
			continue;
		}
	}
}

// Assume whar *cursor point to the start of first command
// Move *cursor to first command delimeter, meta character, pipeline delim or
// if end of string is reached return PARSING_EOL
static parsing_return_code parse_command(char **cursor, command *cmd){
	if (IS_EOL(**cursor)){
		fprintf(stderr, "Unexpected end of line\n");
		return PARSING_ERROR;
	}
	if (IS_BLANK(**cursor) || IS_META(**cursor) || IS_COMMAND_DELIM(**cursor) || IS_PIPELINE_DELIM(**cursor)){
		fprintf(stderr, "Expected start of command, got: \"%c\"\n", **cursor);
		return PARSING_ERROR;
	}

	while (1){
		parsing_return_code ret = parse_arg(cursor, cmd);
		
		if (ret == PARSING_ERROR || ret == PARSING_EOL){
			return ret;
		}

		if (IS_PIPELINE_DELIM(**cursor) || IS_META(**cursor) || IS_COMMAND_DELIM(**cursor)){
			return PARSING_UNFINISHED;
		}
	}
}

// Assume what *cursor points to the start of arg
// Move *cursor to first non-blank symbol after this argument
// if end of string is reached return PARSING_EOL
static parsing_return_code parse_arg(char **cursor, command *cmd){
	char *arg;
	parsing_return_code ret = get_arg(cursor, &arg);

	if (ret == PARSING_ERROR){
		return PARSING_ERROR;
	}

	command_add_arg(cmd, arg);

	return ret;
}

// Processes meta symbol and it's args
// Move *cursor to first nob-blank symbol after meta
// if end of string is reached return PARSING_EOL
static parsing_return_code process_meta(char **cursor, pipeline *pipl){
	if (**cursor == '<'){
		(*cursor)++;
		skip_blank(cursor);

		char *arg;
		parsing_return_code ret = get_arg(cursor, &arg);

		if (ret == PARSING_ERROR){
			return PARSING_ERROR;
		}

		pipeline_set_infile(pipl, arg);

		return ret;
	}
	else if (**cursor == '>'){
		if (*(*cursor + 1) == '>'){
			(*cursor) += 2;
			skip_blank(cursor);

			char *arg;
			parsing_return_code ret = get_arg(cursor, &arg);

			if (ret == PARSING_ERROR){
				return PARSING_ERROR;
			}

			pipeline_set_appfile(pipl, arg);

			return ret;
		}
		else{
			(*cursor)++;
			skip_blank(cursor);

			char *arg;
			parsing_return_code ret = get_arg(cursor, &arg);

			if (ret == PARSING_ERROR){
				return PARSING_ERROR;
			}

			pipeline_set_outfile(pipl, arg);

			return ret;
		}
	}
	else{
		fprintf(stderr, "Expected meta character, got: \"%c\"\n", **cursor);
		return PARSING_ERROR;
	}
}


// Moves provided pointer to:
// - First non-blank char
// - End of string
static void skip_blank(char **cursor){
	while (!IS_EOL(**cursor) && (IS_BLANK(**cursor))){
		(*cursor)++;
	}
}

// Assume what *cursor points to the start of arg
// Returns substring, representing single word
// Moves cursor to first non-blank symbol after this word or
// Returns PARSING_EOF if end of string is reached
static parsing_return_code get_arg(char **cursor, char **out){
	if (IS_EOL(**cursor)){
		fprintf(stderr, "Unexpected end of line\n");
		return PARSING_ERROR;
	}
	if (IS_BLANK(**cursor) || IS_META(**cursor) || IS_COMMAND_DELIM(**cursor) || IS_PIPELINE_DELIM(**cursor)){
		fprintf(stderr, "Expected argument, got: \"%c\"\n", **cursor);
		return PARSING_ERROR;
	}

	char *arg_start = *cursor;

	bool escaping_by_backslash = false, escaping_by_quotes = false;

	while (1){
		if (**cursor == '\\' && !escaping_by_backslash){
			escaping_by_backslash = true;
			(*cursor)++;
			continue;
		}
		else if (**cursor == '"' && !escaping_by_backslash){
			escaping_by_quotes = !escaping_by_quotes;
		}
		else if (IS_EOL(**cursor) || ((IS_BLANK(**cursor) || IS_PIPELINE_DELIM(**cursor)
			|| IS_META(**cursor) || IS_COMMAND_DELIM(**cursor))
				&& !escaping_by_quotes && !escaping_by_backslash)){
			break;
		}

		if (escaping_by_backslash){
			escaping_by_backslash = false;
		}

		(*cursor)++;
	}

	// Quotes isn't closed
	if (escaping_by_quotes){
		fprintf(stderr, "Unclosed quotes\n");
		return PARSING_ERROR;
	}

	*out = filter_arg(arg_start, *cursor - arg_start);

	skip_blank(cursor);

	if (IS_EOL(**cursor)){
		return PARSING_EOL;
	}

	return PARSING_UNFINISHED;
}

static char* filter_arg(char *start, size_t size){
	charstream arg_stream = charstream_open();

	bool escaping_by_backslash = false, escaping_by_quotes = false;

	for (size_t i = 0; i < size; i++){
		if (*start == '\\'){
			if (escaping_by_backslash || (escaping_by_quotes && ((i + 1) >= size || *(start + 1) != '"'))){
				charstream_push(&arg_stream, '\\');
			}
			else{
				escaping_by_backslash = true;
				start++;
				continue;
			}
		}
		else if (*start == '"'){
			if (escaping_by_backslash){
				charstream_push(&arg_stream, '"');
			}
			else{
				escaping_by_quotes = !escaping_by_quotes;
			}
		}
		else{
			charstream_push(&arg_stream, *start);
		}

		if (escaping_by_backslash){
			escaping_by_backslash = false;
		}

		start++;
	}

	charstream_push(&arg_stream, '\0');

	return charstream_close(&arg_stream);
}

