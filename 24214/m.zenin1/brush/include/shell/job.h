#pragma once

#include <stdbool.h>
#include <termios.h>
#include <stdbool.h>
#include <stddef.h>
#include "ptr_vec.h"
#include "parse/pipeline.h"
#include "shell/process.h"

typedef enum job_status {
	NOT_LAUNCHED,
	RUNNING,
	STOPPED,
	COMPLETED
} job_status;

typedef struct job {
	ptr_vec processes;
	pid_t pgid;
	char *original_line;
	struct termios term_attrs;
	bool foreground;
	job_status status;
	bool notified;
	char *infile;
	char *outfile;
	char *errfile;
	bool append_out;
	size_t index;
	struct job *next;
} job;

job job_construct();
job job_get_from_pipeline(pipeline *pipl);
char* job_get_status_string(job *j);
process* job_get_process(job *j, size_t index);
int job_launch(job *j);
bool job_check_status(job *j);
bool job_is_terminated_by_sig(job *j);
void job_destruct(job *j);

