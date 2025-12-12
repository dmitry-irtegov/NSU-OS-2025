#include <stdlib.h>
#include <stddef.h>
#include <sys/wait.h>
#include "shell/job_list.h"
#include "shell/shell.h"

static size_t job_list_next_index(job_list *jl);

job_list job_list_construct(){
	job *sentinel = malloc(sizeof(job));
	*sentinel = job_construct();

	return (job_list) {sentinel, sentinel};
}

job* job_list_add(job_list *jl, job j){
	job *new_job = malloc(sizeof(job));
	*new_job = j;
	new_job->index = job_list_next_index(jl);

	jl->end->next = new_job;
	jl->end = new_job;

	return new_job;
}

void job_list_remove_next(job_list *jl, job *j){

	if (j == NULL || j->next == NULL){
		return;
	}

	job *to_remove = j->next;

	if (jl->end == to_remove){
		jl->end = j;
	}

	j->next = to_remove->next;
	job_destruct(to_remove);
	free(to_remove);

}

// If process_job != NULL save pointer to job, where process belongs, to *process_job
process* job_list_find_process(job_list *jl, pid_t pid, job **process_job){
	job *cursor = jl->start->next;

	while (cursor != NULL){

		for (size_t i = 0; i < cursor->processes.size; i++){
			if (job_get_process(cursor, i)->pid == pid){
				if (process_job){
					*process_job = cursor;
				}
				return job_get_process(cursor, i);
			}
		}

		cursor = cursor->next;
	}

	return NULL;
}

job* job_list_get_job_by_id(job_list *jl, size_t id){
	job *cursor = jl->start->next;

	while (cursor != NULL){
		if (cursor->index == id){
			return cursor;
		}

		cursor = cursor->next;
	}

	return NULL;
}

// Also checks job status
void job_list_set_process_status(job_list *jl, pid_t pid, int status){
	job *process_job;
	process *proc = job_list_find_process(jl, pid, &process_job);

	if (proc == NULL){
		return;
	}

	proc->status = status;

	if (WIFSTOPPED(status)){
		proc->stopped = true;
	}
	else if (WIFEXITED(status) || WIFSIGNALED(status)){
		proc->completed = true;
	}
	else if (WIFCONTINUED(status)){
		proc->stopped = false;
	}

	if (job_check_status(process_job)){
		process_job->notified = false;
	}
}

void job_list_destruct(job_list *jl){
	job *cursor = jl->start;

	while (cursor != NULL){
		job *next = cursor->next;
		job_destruct(cursor);
		free(cursor);
		cursor = next;
	}
}

static size_t job_list_next_index(job_list *jl){
	return jl->end->index + 1;
}

