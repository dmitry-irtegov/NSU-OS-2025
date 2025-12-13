#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "job_list.h"
#include "pipeline.h"
#include "process.h"
#include "job.h"
#include <signal.h>
#include <sys/wait.h>

static struct termios term_attrs;

job_t *job_init() {
	job_t *job = malloc(sizeof(job_t));
  if (!job) {
  	perror("Failed to allocate memory for job");
    return NULL;
  }

	*job = (job_t) {
		.id = 0,
		.status = JOB_RUNNING,
		.pgid = 0,
		.process_count = 0,
		.next = NULL
	};

	tcgetattr(STDIN_FILENO, &job->term_attrs);
	
	return job;
 }

void launch_job(job_list_t *list, job_t *job, pipeline_t *pipeline) {
	int mypipe[2], infile = STDIN_FILENO, outfile;
	mypipe[0] = -1;
	mypipe[1] = -1;

	if (pipeline->infile) {
		infile = open(pipeline->infile, O_RDONLY);
		if (infile == -1) {
			perror("Failed to open file.");
			exit(1);
		}
	}

	for (int i = 0; i < pipeline->cmd_count; i++) {
		if (i + 1 < pipeline->cmd_count) {
			if (pipe(mypipe)) {
				perror("Failed to setup pipe");
				exit(1);
			}
			outfile = mypipe[1];
		} else {
			outfile = STDOUT_FILENO;
		}

		if (i == pipeline->cmd_count - 1 && (pipeline->outfile || pipeline->appfile)) {
			if (pipeline->outfile) {
				outfile = open(pipeline->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			} else {
				outfile = open(pipeline->appfile, O_WRONLY | O_CREAT | O_APPEND, 0666);
			}
			if (outfile < 0) {
				perror("Failed to open file.");
				exit(1);
			}
		}
		pid_t pid = fork();
		switch(pid) {
			case -1:
				perror("Failed to launch child process");
				exit(1);
			case 0:
				if (mypipe[0] != -1 && mypipe[0] != infile) {
					close(mypipe[0]);
				}
				if (mypipe[1] != -1 && mypipe[1] != outfile) {
					close(mypipe[1]);
				}

				launch_process(&pipeline->cmds[i], job->pgid, infile, outfile, pipeline->foreground);
			default:
				if (job->pgid == 0) {
					job->pgid = pid;
				}
				setpgid(pid, job->pgid);
				process_init(&job->processes[i], pid);
				job->process_count++;
				if (infile != STDIN_FILENO) {
        	close(infile);
				}
      	if (outfile != STDOUT_FILENO) {
        	close(outfile);
				}
				infile = mypipe[0];
			}
	}
	if (pipeline->foreground) {
		put_job_in_fg(list, job, 0);
	} else {
    fprintf(stderr, "Background pid [%d]\n",job->pgid);
	}
}

void wait_for_foreground_job(job_list_t *list, job_t *job) {
	int status;
	pid_t pid;
	do  {
		pid = waitpid(-job->pgid, &status, WUNTRACED);
		 if (pid <= 0) {
		 	break;
		 }
		update_job_status(job, status);
	} while (job->status == JOB_RUNNING);


	if (job->status == JOB_STOPPED) {
		fprintf(stderr, "[Stopped job %d]\n", job->id);
		tcgetattr(STDIN_FILENO, &job->term_attrs);
		return;
	}

	if (job->status == JOB_DONE) {
		job_list_remove(list, job->id);
	}
}

void wait_for_background_job(job_list_t *list, job_t *job) {
	int status;
	pid_t pid = waitpid(-job->pgid, &status, WUNTRACED | WNOHANG);
  if (pid > 0) {
  	update_job_status(job, status);
		if (job->status == JOB_DONE) {
      job_list_remove(list, job->id);
		}
	}
}

void check_background_jobs(job_list_t *list) {
	job_t *current = list->start;
  job_t *next;
    
  while (current) {
  	next = current->next;  
  	if (current->status == JOB_RUNNING) {
			wait_for_background_job(list, current);
		}
	  current = next;
	}
}

void update_job_status(job_t *job, int status) {
	job->status = JOB_DONE;
	for (int i = 0; i < job->process_count; i++) {
		update_process_status(&job->processes[i], status);
		switch (job->processes[i].status) {
			case PROCESS_STOPPED:
				job->status = JOB_STOPPED;
				break;
			case PROCESS_COMPLETED:
				continue;
			default:
				job->status = JOB_RUNNING;
		}
	}
}

void put_job_in_fg(job_list_t *list, job_t *job, bool cont) {
	job->status = JOB_RUNNING;

	if (tcgetattr(0, &term_attrs)) {
  	perror("Could not save terminal attributes");
    return;
  }

	if (tcsetpgrp(STDIN_FILENO, job->pgid)) {
		perror("Could not put job to foreground");
		return;
	}

	if (cont) {
		fprintf(stderr, "Putting job [%d] %d to foreground.\n", job->id, job->pgid);
		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &job->term_attrs)) {
			perror("Could not restore terminal attributes");
			return;
		}
		if (kill(-job->pgid, SIGCONT)) {
			perror("Failed to continue foreground job");
		}
	}
	
	wait_for_foreground_job(list, job);

	if (tcsetpgrp(STDIN_FILENO, getpgrp())) {
		perror("Could not set foreground group");
		return;
	}

	if (tcsetattr(STDIN_FILENO, TCSADRAIN, &term_attrs)) {
		perror("Could not restore terminal attributes");
		return;
	}
}

void put_job_in_bg(job_t *job) {
	job->status = JOB_RUNNING;
	fprintf(stderr, "Putting job [%d] with pid %d to background.\n", job->id, job->pgid);
	if (kill(-job->pgid, SIGCONT)) {
		perror("Failed to continue background job");
	}
}

