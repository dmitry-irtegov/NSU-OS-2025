#include "builtins.h"
#include "job.h"
#include "pipeline.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void execute_fg(command_t *command, job_list_t *list);
static void execute_bg(command_t *command, job_list_t *list);
static void execute_jobs(command_t *command, job_list_t *list);

builtin_t builtins[] = {
	{"fg", execute_fg},
	{"bg", execute_bg},
	{"jobs", execute_jobs},
	{NULL}
};

bool try_execute_builtin(pipeline_t *pipeline, job_list_t *list) {
	for (const builtin_t *builtin = builtins; builtin->name; builtin++) {
		if (strcmp(pipeline->cmds[0].cmdargs[0], builtin->name) == 0) {
			builtin->function(pipeline->cmds, list); 
			return 1;
		}
	}
	return 0;
}

static void execute_fg(command_t *command, job_list_t *list) {
	job_t *current = list->start;
	if (command->cmdargs[1] == NULL) {
		while (current && current->status == JOB_DONE) { 
			current = current->next;
		}
	} else {
		int job_id = atoi(command->cmdargs[1]);
    current = find_job_by_id(list, job_id);
	}
	if (!current) {
		fprintf(stderr, "fg: no current job\n");
		return;
	}
	put_job_in_fg(list, current, 1);
}

static void execute_bg(command_t *command, job_list_t *list) {
	job_t *current = list->start;
	if (command->cmdargs[1] == NULL) {
		while (current && current->status == JOB_DONE) { 
			current = current->next;
		}
	} else {
		int job_id = atoi(command->cmdargs[1]);
    current = find_job_by_id(list, job_id);
	}
	if (!current) {
		fprintf(stderr, "bg: no current job\n");
		return;
	}

	put_job_in_bg(list, current);
}

static void execute_jobs(command_t *command, job_list_t *list) {
	job_list_print(list);
}

