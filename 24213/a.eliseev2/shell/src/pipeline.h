#ifndef __PIPELINE_H
#define __PIPELINE_H

#include <sys/types.h>

#define MAXCMD 50
#define MAXARGS 256

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
pid_t launch_pipeline(pipeline_t *pipeline);

#endif // __PIPELINE_H
