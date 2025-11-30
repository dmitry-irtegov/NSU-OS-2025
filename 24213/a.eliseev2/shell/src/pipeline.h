#ifndef __PIPELINE_H
#define __PIPELINE_H

#include "shell_limits.h"
#include <sys/types.h>

typedef struct {
    char *args[MAXARGS];
} command_t;

typedef enum {
    PLAPPEND = 0x01,
    PLBKGRND = 0x02,
} plflag_t;

typedef struct {
    command_t commands[MAXCMD];
    int cmd_count;
    char *in_file;
    char *out_file;
    plflag_t flags;
} pipeline_t;

int parse_pipeline(char **line_ptr, pipeline_t *pipeline);

#endif // __PIPELINE_H
