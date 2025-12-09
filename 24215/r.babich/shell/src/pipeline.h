#pragma once
#include <stdbool.h>

#define MAXARGS 256
#define MAXCMDS 50

/* cmdflag's */
#define OUTPIP 01
#define INPIP 02

typedef struct {
	char *cmdargs[MAXARGS];
	char cmdflag;
} command_t;

typedef struct {
	command_t cmds[MAXCMDS];
	int cmd_count;
	bool foreground;
	char *infile;
	char *outfile;
	char *appfile;
} pipeline_t;

int parse_pipeline(char*, pipeline_t *pipeline);
int promptline(char*, char*, int);

