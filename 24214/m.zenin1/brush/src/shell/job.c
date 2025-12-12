#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <stdint.h>
#include <errno.h>
#include <sys/wait.h>
#include "shell/job.h"
#include "shell/process.h"
#include "shell/shell.h"
#include "ptr_vec.h"

static bool job_is_stopped(job *j);
static bool job_is_completed(job *j);
// Return true if special, else false
static bool try_execute_special_command(char **argv, int infile, int outfile, int errfile, bool foreground);
static void execute_fg(char **argv, int infile, int outfile, bool foreground);
static void execute_bg(char **argv, int infile, int outfile, bool foreground);

job job_construct(){
	job out;
	out.processes = ptr_vec_construct();
	out.pgid = 0;
	out.original_line = NULL;
	tcgetattr(STDIN_FILENO, &out.term_attrs);
	out.foreground = false;
	out.status = NOT_LAUNCHED;
	out.notified = true;
	out.infile = NULL;
	out.outfile = NULL;
	out.errfile = NULL;
	out.append_out = false;
	out.index = 0;
	out.next = NULL;

	return out;
}

// Get infile, outfile and errfile right from pipeline
job job_get_from_pipeline(pipeline *pipl){
	job out;
	out.processes = ptr_vec_construct();
	out.pgid = 0;

	out.original_line = pipl->original_line;
	pipl->original_line = NULL;

	out.foreground = pipl->foreground;
	out.status = NOT_LAUNCHED;
	out.notified = true;

	tcgetattr(STDIN_FILENO, &out.term_attrs);

	out.infile = pipl->infile;
	pipl->infile = NULL;	

	if (pipl->appfile){
		out.outfile = pipl->appfile;
		pipl->appfile = NULL;
		out.append_out = true;
	}
	else{
		out.outfile = pipl->outfile;
		pipl->outfile = NULL;
		out.append_out = false;
	}

	out.errfile = NULL;

	out.next = NULL;

	for (size_t i = 0; i < pipl->commands.size; i++){
		process *new_process = malloc(sizeof(process));
		*new_process = process_get_from_command(pipl->commands.arr[i]);

		ptr_vec_push(&out.processes, new_process);
	}

	out.index = 0;

	return out;
}

char* job_get_status_string(job *j){
	switch (j->status){
		case NOT_LAUNCHED:
			return "not launched";
		case RUNNING:
			return "running";
		case STOPPED:
			return "stopped";
		case COMPLETED:
			return "completed";
		default:
			return "unknown";
	}
}

// Returns NULL if index >= j->processes.size
process* job_get_process(job *j, size_t index){
	if (index >= j->processes.size){
		return NULL;
	}

	return (process*) j->processes.arr[index];
}

// returns 0 on success, otherwise - -1
int job_launch(job *j){
	int job_infile = STDIN_FILENO, job_outfile = STDOUT_FILENO, job_errfile = STDERR_FILENO;

	/*
	if (j->infile){
		job_infile = open(j->infile, 0, O_RDONLY);
		if (job_infile == -1){
			perror("Failed to redirect input");
			return -1;
		}
	}
	if (j->outfile){
		if (j->append_out){
			job_outfile = open(j->outfile, O_CREAT | O_APPEND | O_WRONLY, 0664);
		}
		else{
			job_outfile = open(j->outfile, O_CREAT | O_WRONLY, 0664);
		}
		if (job_outfile == -1){
			perror("Failed to redirect output");
			return -1;
		}
	}
	if (j->errfile){
		job_errfile = open(j->errfile, O_CREAT);
		if (job_errfile == -1){
			perror("Failed to redirect errors");
			return -1;
		}
	}
	*/

	int mypipe[2], infile, outfile;
	mypipe[0] = -1;
	mypipe[1] = -1;
	infile = job_infile;

	bool real_job_launched = false;

	// Run all processes
	for (size_t i = 0; i < j->processes.size; i++){
		// Setup pipe if there is next process
		if (i + 1 < j->processes.size){
			if (pipe(mypipe)){
				perror("Failed to setup pipe");
				return -1;
			}
			outfile = mypipe[1];
		}
		else{
			outfile = job_outfile;
		}

		if (!try_execute_special_command(job_get_process(j, i)->argv, infile, outfile, job_errfile, j->foreground)){
			real_job_launched = true;

			pid_t child_pid = fork();
			if (child_pid == -1){
				perror("Failed to fork new process");
				return -1;
			}

			// Child
			if (child_pid == 0){
				signal(SIGTSTP, SIG_DFL);
				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
				if (mypipe[0] != -1 && mypipe[0] != infile){
					close(mypipe[0]);
				}
				if (mypipe[1] != -1 && mypipe[1] != outfile){
					close(mypipe[1]);
				}
				if (infile == job_infile && j->infile){
					infile = open(j->infile, 0, O_RDONLY);
					if (infile == -1){
						perror("Failed to redirect input");
						return -1;
					}

				}
				if (outfile == job_outfile && j->outfile){
					if (j->append_out){
						outfile = open(j->outfile, O_CREAT | O_APPEND | O_WRONLY, 0664);
					}
					else{
						outfile = open(j->outfile, O_CREAT | O_WRONLY, 0664);
					}
					if (outfile == -1){
						perror("Failed to redirect output");
						return -1;
					}
				}
				if (j->errfile){
					job_errfile = open(j->errfile, O_CREAT);
					if (job_errfile == -1){
						perror("Failed to redirect errors");
						return -1;
					}
				}

				process_run(job_get_process(j, i), j->pgid, infile, outfile, job_errfile, j->foreground);
			}

			// Shell
			job_get_process(j, i)->pid = child_pid;
			if (j->pgid == 0){
				j->pgid = child_pid;
			}
			if (setpgid(child_pid, j->pgid)){
				perror("Failed to change pgid");
				return -1;
			}
		}
		else{
			job_get_process(j, i)->completed = true;
			job_get_process(j, i)->status = 0;
		}

		// Close pipe ends, used by runned process
		if (infile != job_infile){
			close(infile);
		}
		if (outfile != job_outfile){
			close(outfile);
		}
		infile = mypipe[0];
	}

	/*
	// Cleanup job file descriptors
	if (job_errfile != STDERR_FILENO){
		close(job_errfile);
	}
	if (job_outfile != STDOUT_FILENO){
		close(job_outfile);
	}
	if (job_infile != STDIN_FILENO){
		close(job_infile);
	}
	*/

	if (job_check_status(j)){
		j->notified = false;
	}

#ifdef DEBUG
	shell_print_job_info(j);
#endif

	if (!real_job_launched){
		return 0;
	}

	// Wait 'til job is done or put it on backgroud
	if (j->foreground){
		shell_put_job_on_foreground(j);
	}
	else{
		printf("[%zu] %ju\n", j->index, (uintmax_t) j->pgid);
		shell_put_job_on_background(j);
	}

	return 0;
}

void job_destruct(job *j){
	for (size_t i = 0; i < j->processes.size; i++){
		process_destruct(job_get_process(j, i));
		free(job_get_process(j, i));
	}
	ptr_vec_destruct(&j->processes);

	if (j->infile){
		free(j->infile);
	}
	if (j->outfile){
		free(j->outfile);
	}
	if (j->errfile){
		free(j->errfile);
	}
	if (j->original_line){
		free(j->original_line);
	}
}

static bool try_execute_special_command(char **argv, int infile, int outfile, int errfile, bool foreground){
	if (argv[0] == NULL){
		return false;
	}

	if (strcmp(argv[0], "exit") == 0){
		shell_exit(0);

		// Actualy there is no return from shell_exit, but anyways...
		return true;
	}
	else if (strcmp(argv[0], "jobs") == 0){
		shell_print_all_jobs();
		return true;
	}
	else if (strcmp(argv[0], "bg") == 0){
		execute_bg(argv, infile, outfile, foreground);
		return true;
	}
	else if (strcmp(argv[0], "fg") == 0){
		execute_fg(argv, infile, outfile, foreground);
		return true;
	}

	return false;
}

static void execute_fg(char **argv, int infile, int outfile, bool foreground){
	if (infile != STDIN_FILENO || outfile != STDOUT_FILENO ||  !foreground){
		fprintf(stderr, "Cannot do job control\n");
		return;
	}

	job *job_to_put_on_fg = NULL;

	if (argv[1] == NULL){
		job_to_put_on_fg = current_job;
	}
	else{
		char *end;
		unsigned long index = strtoul(argv[1], &end, 10);
#ifdef DEBUG
		printf("Trying to put on fg job %ld\n", index);
#endif
		errno = 0;
		if (*end != '\0' || errno){
			fprintf(stderr, "Invalid argument for fg, expected positive int\n");
			return;
		}
		job_to_put_on_fg = job_list_get_job_by_id(&active_jobs, (size_t) index);
		job_to_put_on_fg = job_to_put_on_fg->status == NOT_LAUNCHED ? NULL : job_to_put_on_fg;
	}

	if (job_to_put_on_fg == NULL){
		fprintf(stderr, "Specified job doesn't exist\n");
		return;
	}

	shell_put_job_on_foreground(job_to_put_on_fg);
}

static void execute_bg(char **argv, int infile, int outfile, bool foreground){
	if (infile != STDIN_FILENO || outfile != STDOUT_FILENO || !foreground){
		fprintf(stderr, "Cannot do job control\n");
		return;
	}
	job *job_to_put_on_bg = NULL;

	if (argv[1] == NULL){
		job_to_put_on_bg = current_job;
	}
	else{
		char *end;
		unsigned long index = strtoul(argv[1], &end, 10);
#ifdef DEBUG
		fprintf(stdout, "Trying to put on bg job %ld\n", index);
#endif
		errno = 0;
		if (*end != '\0' || errno){
			fprintf(stderr, "Invalid argument for bg, expected positive int\n");
			return;
		}
		job_to_put_on_bg = job_list_get_job_by_id(&active_jobs, (size_t) index);
		job_to_put_on_bg = job_to_put_on_bg->status == NOT_LAUNCHED ? NULL : job_to_put_on_bg;
	}

	if (job_to_put_on_bg == NULL){
		fprintf(stderr, "Specified job doesn't exist\n");
		return;
	}

	shell_put_job_on_background(job_to_put_on_bg);
}

// Return true, if job completed and first or last process in pipeline was terminated by signal
// Because this mean, what they might leave terminal with strange attributes
bool job_is_terminated_by_sig(job *j){
	// Just in case
	if (j->processes.size == 0){
		return false;
	}
	return j->status == COMPLETED && (WIFSIGNALED(((process*)j->processes.arr[0])->status) || WIFSIGNALED(((process*)j->processes.arr[j->processes.size - 1])->status));
}

bool job_check_status(job *j){
	bool ret = false;
	if (job_is_completed(j)){
		ret = (j->status != COMPLETED);
		j->status = COMPLETED;
	}
	else if (job_is_stopped(j)){
		ret = (j->status != STOPPED);
		j->status = STOPPED;
	}
	else{
		ret = (j->status != RUNNING);
		j->status = RUNNING;
	}

	return ret;
}

static bool job_is_stopped(job *j){
	for (size_t i = 0; i < j->processes.size; i++){
		if (!job_get_process(j, i)->completed && !job_get_process(j, i)->stopped){
			return false;
		}
	}

	return true;
}

static bool job_is_completed(job *j){
	for (size_t i = 0; i < j->processes.size; i++){
		if (!job_get_process(j, i)->completed){
			return false;
		}
	}

	return true;
}

