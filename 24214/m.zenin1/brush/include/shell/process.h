#pragma once

#include <unistd.h>
#include <stdbool.h>
#include "parse/command.h"

typedef struct process {
	char **argv;
	pid_t pid;
	bool completed;
	bool stopped;
	int status;
} process;

process process_get_from_command(command *cmd);
void process_run(process *p, pid_t pgid, int infile, int outfile, int errfile, bool foreground);
void process_destruct(process *p);

