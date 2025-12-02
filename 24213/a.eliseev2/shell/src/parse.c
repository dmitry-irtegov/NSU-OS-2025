#include "parse.h"

#include <ctype.h>
#include <stdio.h>

typedef enum {
    TOK_WORD,
    TOK_IN,
    TOK_OUT,
    TOK_APPEND,
    TOK_PIPE,
    TOK_NEXT,
    TOK_BGND,
    TOK_EOL,
} toktype_t;

typedef struct {
    toktype_t type;
    char *content; // Only valid for TOK_WORD
} token_t;

static const token_t tok_eol = (token_t){.type = TOK_EOL};

static const char *tokstr(token_t token) {
    switch (token.type) {
    case TOK_WORD:
        return "WORD";
    case TOK_IN:
        return "IN";
    case TOK_OUT:
        return "OUT";
    case TOK_APPEND:
        return "APPEND";
    case TOK_PIPE:
        return "PIPE";
    case TOK_NEXT:
        return "NEXT";
    case TOK_BGND:
        return "BACKGROUND";
    case TOK_EOL:
        return "EOL";
    default:
        return "(unknown)";
    }
}

static void skip_whitespace(parser_t *parser) {
    while (1) {
        char ch = parser->point[0];

        switch (ch) {
        case '\0':
            return;
        case '\\':
            if (parser->point[1] == '\n') {
                break;
            }
            return;
        default:
            if (isspace(ch)) {
                break;
            }
            return;
        }
        parser->point++[0] = '\0';
    }
}

static token_t read_word(parser_t *parser) {
    token_t token = (token_t){.type = TOK_WORD, .content = parser->point};
    while (1) {
        char ch = parser->point[0];

        switch (ch) {
        case '&':
        case ';':
        case '>':
        case '<':
        case '|':
        case '\0':
            return token;
        case '\\':
            if (parser->point[1] != '\n') {
                break;
            }
            return token;
        default:
            if (!isspace(ch)) {
                break;
            }
            return token;
        }
        parser->point++;
    }
}

static token_t read_char(parser_t *parser, toktype_t type) {
    parser->point++[0] = '\0';
    return (token_t){.type = type};
}

static token_t read_out(parser_t *parser) {
    if (parser->point[1] == '>') {
        parser->point++[0] = '\0';
        parser->point++[0] = '\0';
        return (token_t){.type = TOK_APPEND};
    } else {
        parser->point++[0] = '\0';
        return (token_t){.type = TOK_OUT};
    }
}

static token_t read_token(parser_t *parser) {
    skip_whitespace(parser);
    switch (parser->point[0]) {
    case '\0':
        return tok_eol;
    case '|':
        return read_char(parser, TOK_PIPE);
    case '&':
        return read_char(parser, TOK_BGND);
    case ';':
        return read_char(parser, TOK_NEXT);
    case '<':
        return read_char(parser, TOK_IN);
    case '>':
        return read_out(parser);
    default:
        return read_word(parser);
    }
}

static int ensure_word(token_t token) {
    if (token.type == TOK_WORD) {
        return 0;
    }
    fprintf(stderr, "Syntax error: unexpected token %s in I/O redirection.\n",
            tokstr(token));
    return 1;
}

static int finilize_command(pipeline_t *pipeline, int *arg_count) {
    if (*arg_count == 0) {
        fprintf(stderr, "Syntax error: no command arguments\n");
        return 1;
    }
    if (pipeline->cmd_count == MAXCMD) {
        fprintf(stderr, "Too many commands in a pipeline!\n");
        return 1;
    }
    pipeline->commands[pipeline->cmd_count++].args[*arg_count] = NULL;
    *arg_count = 0;
    return 0;
}

int parse_pipeline(parser_t *parser, pipeline_t *pipeline) {
    pipeline->cmd_count = 0;
    pipeline->in_file = NULL;
    pipeline->out_file = NULL;
    pipeline->flags = 0;

    int arg_count = 0;

    token_t token = read_token(parser);
    if (token.type == TOK_EOL) {
        // Return immediately if the line is empty
        return 0;
    }

    do {
        token_t next;

        switch (token.type) {
        case TOK_WORD: {
            if (arg_count == MAXARGS) {
                fprintf(stderr, "Too many arguments in a command!\n");
                return 0;
            }
            pipeline->commands[pipeline->cmd_count].args[arg_count++] =
                token.content;
        } break;
        case TOK_APPEND:
            pipeline->flags |= PLAPPEND;
        case TOK_OUT:
            next = read_token(parser);
            if (ensure_word(next)) {
                return 0;
            }
            pipeline->out_file = next.content;
            break;
        case TOK_IN:
            next = read_token(parser);
            if (ensure_word(next)) {
                return 0;
            }
            pipeline->in_file = next.content;
            break;
        case TOK_BGND:
            pipeline->flags |= PLBKGRND;
        case TOK_NEXT:
            if (finilize_command(pipeline, &arg_count)) {
                return 0;
            }
            return 1;
        case TOK_PIPE:
            if (finilize_command(pipeline, &arg_count)) {
                return 0;
            }
            break;
        default:
            // Should be unreachable
            break;
        }
        token = read_token(parser);
    } while (token.type != TOK_EOL);
    
    if (finilize_command(pipeline, &arg_count)) {
        return 0;
    }

    return 1;
}
