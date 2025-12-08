#include "job_list.h"
#include "job.h"
#include <stdio.h>
#include <stdlib.h>

void job_list_init(job_list_t *list) {
	list->start = NULL;
	list->end = NULL;
}

void job_list_add(job_list_t *list, job_t *job) {
	if (list->start == NULL) {
		job->id = 0;
		list->start = job;
	} else {
		job->id = list->end->id + 1; 
		list->end->next = job;
	}
	list->end = job;
}

void job_list_remove(job_list_t *list, int id) {
	if (!list->start) {
		return;
	}
    
  job_t *current = list->start;
  job_t *prev = NULL;
    
  while (current) {
  	if (current->id == id) {
    	if (prev) {
      	prev->next = current->next;
      } else {
      	list->start = current->next;
      }
            
      if (current == list->end) {
      	list->end = prev;
      }
			free(current); 
			return;
		}
		prev = current;
		current = current->next;
	}
}

job_t *find_job_by_id(job_list_t *list, int job_id) {
	job_t *current = list->start;

  while (current) {
  	if (current->id == job_id) {
			return current;
		}
    current = current->next;
  }

	return NULL;
}

void job_list_print(job_list_t *list) {
	job_t *current = list->start;
  while (current) {
		char *status;

		switch (current->status) {
			case JOB_RUNNING:
				status = "running";
				break;
			case JOB_DONE:
				status = "done";
				break;
			case JOB_STOPPED:
				status = "stopped";
				break;
		}

		fprintf(stderr, "[%d]    %s\n", current->id, status);
  	current = current->next;
  }
}

