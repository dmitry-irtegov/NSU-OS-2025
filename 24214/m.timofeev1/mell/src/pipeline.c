#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include "shell.h"

extern pid_t shell_pgid;
extern struct termios shell_tmodes;
extern int shell_terminal;
extern int shell_is_interactive;
extern char *infile, *outfile, *appfile;
extern char bkgrnd;

int execute_pipeline(int ncmds, char *cmdline)
{
	int i;
	int pipes[MAXCMDS][2];
	pid_t pgid = 0;
	pid_t pids[MAXCMDS];

	for (i = 0; i < ncmds - 1; i++)
	{
		if (pipe(pipes[i]) < 0)
		{
			perror("pipe");
			return -1;
		}
	}

	for (i = 0; i < ncmds; i++)
	{
		pid_t pid = fork();

		if (pid < 0)
		{
			perror("fork");
			return -1;
		}
		else if (pid == 0)
		{
			if (i == 0)
			{
				pgid = getpid();
			}
			setpgid(0, pgid);

			setup_child_signals();
			setup_io_redirection(i, ncmds, pipes);
			close_all_pipes(ncmds - 1, pipes);
			execute_command(i);
		}
		else
		{
			pids[i] = pid;
			if (i == 0)
			{
				pgid = pid;
			}
			setpgid(pid, pgid);
		}
	}

	close_all_pipes(ncmds - 1, pipes);

	if (!bkgrnd)
	{
		if (shell_is_interactive)
		{
			tcsetpgrp(shell_terminal, pgid);
		}

		add_job(pgid, JOB_RUNNING, cmdline);

		int status;
		int stopped = 0;

		for (i = 0; i < ncmds; i++)
		{
			if (waitpid(pids[i], &status, WUNTRACED) > 0)
			{
				if (WIFSTOPPED(status))
				{
					stopped = 1;
				}
			}
		}

		if (shell_is_interactive)
		{
			tcsetpgrp(shell_terminal, shell_pgid);
			tcgetattr(shell_terminal, &shell_tmodes);
		}

		int job_idx = find_job(pgid);
		if (stopped)
		{
			if (job_idx >= 0)
			{
				update_job_state(job_idx, JOB_STOPPED);
				fprintf(stderr, "[%d] Stopped %s\n", pgid, get_job_cmdline(job_idx));
			}
		}
		else
		{
			if (job_idx >= 0)
			{
				update_job_state(job_idx, JOB_DONE);
			}
		}
	}
	else
	{
		printf("[%d]\n", pgid);
		add_job(pgid, JOB_RUNNING, cmdline);
	}

	return 0;
}
