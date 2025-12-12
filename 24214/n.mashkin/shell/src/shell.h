#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>

#define MAXARGS 256
#define MAXCMDS 50
#define MAXPPLINES 50
#define MAXPATH 100
#define MAXPROMPT 150

typedef struct command {
    char *cmdargs[MAXARGS];
    int nargs;
} command_t;

typedef struct pipeline {
    command_t cmds[MAXCMDS];
    int ncmds;
    char *infile;
    char *outfile;
    char *appfile;
    int bkgrnd;
} pipeline_t;

extern pipeline_t pipelines[MAXPPLINES];
extern pid_t foreground_pgid;
extern pid_t shell_pgid;
extern int terminal_fd;

int parseline(char *);
int promptline(char *, int);

#endif
