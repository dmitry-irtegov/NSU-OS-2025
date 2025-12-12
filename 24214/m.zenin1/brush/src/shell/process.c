#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include "shell/process.h"
#include "parse/command.h"
#include "shell/shell.h"

// Takes args pointers right from command.args
process process_get_from_command(command *cmd){
	process out;
	out.pid = 0;
	out.completed = false;
	out.stopped = false;
	out.status = 0;

	out.argv = malloc((cmd->args.size + 1) * sizeof(char*));

	for (size_t i = 0; i < cmd->args.size; i++){
		out.argv[i] = (char*) cmd->args.arr[i];
	}
	out.argv[cmd->args.size] = NULL;

	// Delete args from command
	cmd->args.size = 0;

	return out;
}

// Execs new programm, specified by p in group with id pgid
// If pgid == 0, creates new group with this process as leader
// infile, outfile, errfile - file descriptors for standart descriptors redirection
// If foreground, new process will set its' group as foreground group of its' controlling terminal
void process_run(process *p, pid_t pgid, int infile, int outfile, int errfile, bool foreground){
	p->pid = getpid();

	// If pgid == 0, new froup will have pgid = p->pid
	if (pgid == 0){
		pgid = p->pid;
	}

	if (setpgid(p->pid, pgid)){
		perror("Failed to change pgid");
		exit(-1);
	}

	// If foreground, set this processes' group as foreground
	if (foreground){
		if (tcsetpgrp(STDIN_FILENO, pgid)){
			perror("Failed to set terminal foreground process group");
			exit(-1);
		}
	}

	// Set job control signals handling
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);

	if (infile != STDIN_FILENO){
		dup2(infile, STDIN_FILENO);
		close(infile);
	}
	if (outfile != STDOUT_FILENO){
		dup2(outfile, STDOUT_FILENO);
		close(outfile);
	}
	if (errfile != STDERR_FILENO){
		dup2(errfile, STDERR_FILENO);
		close(errfile);
	}

	// Exec
	execvp(p->argv[0], p->argv);
	perror("Failed to exec new programm");
	exit(-1);
}

void process_destruct(process *p){
	char **cursor = p->argv;
	while (*cursor){
		free(*cursor);
		cursor++;
	}

	free(p->argv);
}

