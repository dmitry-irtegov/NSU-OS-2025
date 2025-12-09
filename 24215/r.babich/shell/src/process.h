#pragma once

#include "pipeline.h"
#include <sys/types.h>

typedef enum {
	PROCESS_RUNNING,
	PROCESS_COMPLETED,
	PROCESS_STOPPED
} process_status_t;

typedef struct process_t {
	pid_t pid;
	process_status_t status;
} process_t;

void launch_process(command_t *command, int infile, int outfile, int appfile, bool foreground);

void update_process_status(process_t *process, int status);

process_t *process_init(process_t *process, pid_t pid);
