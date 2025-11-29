#include "pipeline.h"
#include <ctype.h>
#include <stdio.h>

static char *skip_whitespace(char *str) {
    while (isspace(*str)) {
        str++;
    }
    return str;
}

static int isarg(char ch) {
    switch (ch) {
    case '&':
    case ';':
    case '>':
    case '<':
    case '|':
    case '\0':
        return 0;
    default:
        return !isspace(ch);
    }
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

int parse_pipeline(char **line_ptr, pipeline_t *pipeline) {
    pipeline->cmd_count = 0;
    pipeline->in_file = NULL;
    pipeline->out_file = NULL;
    pipeline->flags = 0;

    int arg_count = 0;
    char *s = *line_ptr;

    s = skip_whitespace(s);
    if (!*s) {
        return 0;
    }

    while (1) {
        switch (*s) {
        case '&':
            pipeline->flags |= PLBKGRND;
        case ';':
            if (finilize_command(pipeline, &arg_count)) {
                return 0;
            }
            *s++ = '\0';
            *line_ptr = s;
            return 1;
        case '|':
            if (finilize_command(pipeline, &arg_count)) {
                return 0;
            }
            *s++ = '\0';
            break;
        case '>':
            if (*(s + 1) == '>') {
                pipeline->flags |= PLAPPEND;
                *s++ = '\0';
            }
            *s++ = '\0';
            s = skip_whitespace(s);
            if (!isarg(*s)) {
                fprintf(stderr, "Syntax error: no output file name\n");
                return 0;
            }
            pipeline->out_file = s;
            while (isarg(*s)) {
                s++;
            }
            if (isspace(*s)) {
                *s++ = '\0';
            }
            break;
        case '<':
            *s++ = '\0';
            s = skip_whitespace(s);
            if (!isarg(*s)) {
                fprintf(stderr, "Syntax error: no input file name\n");
                return 0;
            }
            pipeline->in_file = s;
            while (isarg(*s)) {
                s++;
            }
            if (isspace(*s)) {
                *s++ = '\0';
            }
            break;
        default:
            if (arg_count == MAXARGS) {
                fprintf(stderr, "Too many arguments in a command!\n");
                return 0;
            }
            pipeline->commands[pipeline->cmd_count].args[arg_count++] = s;
            while (isarg(*s)) {
                s++;
            }
            if (isspace(*s)) {
                *s++ = '\0';
            }
            break;
        }

        s = skip_whitespace(s);
        if (!*s) {
            break;
        }
    }

    if (finilize_command(pipeline, &arg_count)) {
        return 0;
    }
    *line_ptr = s;

    return 1;
}
