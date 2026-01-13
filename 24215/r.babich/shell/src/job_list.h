#pragma once
#include "job.h"

typedef struct job_list_t {
	job_t *start;
	job_t *end;
} job_list_t;

void job_list_init(job_list_t *list);

void job_list_add(job_list_t *list, job_t *job);

void job_list_remove(job_list_t *list, int id);

job_t *find_job_by_id(job_list_t *list, int job_id);

void job_list_print(job_list_t *list);

