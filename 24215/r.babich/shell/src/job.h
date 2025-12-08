#pragma once

#include <sys/types.h>
#include "pipeline.h"
#include "process.h"
#include <termios.h>
#include "sys/wait.h"

typedef enum {
	JOB_RUNNING, 
	JOB_STOPPED,
	JOB_DONE 
} job_status_t; 

typedef struct job_t {
	int id; 
	pid_t pgid; 
	job_status_t status; 
	process_t processes[MAXCMDS]; 
	int process_count;
	struct termios term_attrs;
	struct job_t *next; 
} job_t;

typedef struct job_list_t job_list_t;

void launch_job(job_list_t *list, job_t *job, pipeline_t *pipeline);

void wait_for_foreground_job(job_list_t *list, job_t *job);

void update_job_status(job_t *job, int status);

void put_job_in_fg(job_list_t *list, job_t *job, bool cont);

void put_job_in_bg(job_list_t *list, job_t *job);

void wait_for_background_job(job_list_t *list, job_t *job); 

void check_background_jobs(job_list_t *list);

job_t *job_init();
