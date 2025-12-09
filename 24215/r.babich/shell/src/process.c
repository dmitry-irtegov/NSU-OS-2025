#include "process.h"
#include "pipeline.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

process_t *process_init(process_t *process, pid_t pid) {
	*process = (process_t) {
		.pid = pid,
		.status = PROCESS_RUNNING
	};

	return process;
}

void launch_process(command_t *command, int infile, int outfile, int appfile, bool foreground) {
	if (setpgid(0, 0)){
		perror("Failed to set pgid");
		exit(1);
	}

	if (foreground){
		if (tcsetpgrp(0, getpgrp())){
			perror("Failed to set terminal foreground process group");
			exit(1);
		}
	}

	signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGTTOU, SIG_DFL);
	
	if (infile != STDIN_FILENO) {
    dup2(infile, STDIN_FILENO);
    close(infile);
  }
  if (outfile != STDOUT_FILENO) {
  	dup2(outfile, STDOUT_FILENO);
    close(outfile);
	}
  if (appfile != STDOUT_FILENO) {
  	dup2(appfile, STDOUT_FILENO);
    close(appfile);
  } 
	execvp(command->cmdargs[0], command->cmdargs);
	perror("Execvp failed.");
	exit(1);
}

void update_process_status(process_t *process, int status) {
	process_status_t process_status;
	if (WIFEXITED(status) || WIFSIGNALED(status)) {
		process_status = PROCESS_COMPLETED;
	} else if (WIFSTOPPED(status)) {
		process_status = PROCESS_STOPPED;
	} else if (WIFCONTINUED(status)) {
		process_status = PROCESS_RUNNING;
	} else {
		return;
	}
	process->status = process_status;
}

