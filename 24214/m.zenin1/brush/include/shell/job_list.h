#pragma once

#include <unistd.h>
#include "shell/job.h"

typedef struct job_list {
	job *start;
	job *end;
} job_list;

job_list job_list_construct();
job* job_list_add(job_list *jl, job j);
void job_list_remove_next(job_list *jl, job *j);
process* job_list_find_process(job_list *jl, pid_t pid, job **process_job);
job* job_list_get_job_by_id(job_list *jl, size_t id);
void job_list_set_process_status(job_list *jl, pid_t pid, int status);
void job_list_print_all_jobs(job_list *jl);
void job_list_destruct(job_list *jl);

