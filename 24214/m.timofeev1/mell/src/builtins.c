#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include "shell.h"

int builtin_fg(void)
{
	extern struct job jobs[];
	extern int njobs;
	extern pid_t shell_pgid;
	extern struct termios shell_tmodes;
	extern int shell_terminal;

	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	if (njobs == 0)
	{
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		fprintf(stderr, "fg: no jobs\n");
		return -1;
	}

	int job_idx = -1;
	for (int i = njobs - 1; i >= 0; i--)
	{
		if (jobs[i].state == JOB_STOPPED || jobs[i].state == JOB_RUNNING)
		{
			job_idx = i;
			break;
		}
	}

	if (job_idx < 0)
	{
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		fprintf(stderr, "fg: no suitable job\n");
		return -1;
	}

	pid_t pgid = jobs[job_idx].pgid;

	tcsetpgrp(shell_terminal, pgid);
	if (jobs[job_idx].state == JOB_STOPPED)
	{
		kill(-pgid, SIGCONT);
	}

	jobs[job_idx].state = JOB_RUNNING;

	int status;
	pid_t wait_result;

	while (1)
	{
		wait_result = waitpid(-pgid, &status, WUNTRACED);

		if (wait_result < 0)
		{
			if (errno == ECHILD)
			{
				/* No child processes - probably already reaped by SIGCHLD handler */
				break;
			}
			perror("waitpid in fg");
			break;
		}

		if (WIFEXITED(status) || WIFSIGNALED(status))
		{
			jobs[job_idx].state = JOB_DONE;
			break;
		}

		if (WIFSTOPPED(status))
		{
			jobs[job_idx].state = JOB_STOPPED;
			fprintf(stderr, "[%d] Stopped %s\n", pgid, jobs[job_idx].cmdline);
			break;
		}
	}

	tcsetpgrp(shell_terminal, shell_pgid);
	tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);

	sigprocmask(SIG_SETMASK, &oldmask, NULL);

	if (jobs[job_idx].state == JOB_DONE)
	{
		cleanup_jobs();
	}

	return 0;
}

int builtin_bg(void)
{
	extern struct job jobs[];
	extern int njobs;

	if (njobs == 0)
	{
		fprintf(stderr, "bg: no jobs\n");
		return -1;
	}

	int job_idx = -1;
	for (int i = njobs - 1; i >= 0; i--)
	{
		if (jobs[i].state == JOB_STOPPED)
		{
			job_idx = i;
			break;
		}
	}

	if (job_idx < 0)
	{
		fprintf(stderr, "bg: no stopped jobs\n");
		return -1;
	}

	pid_t pgid = jobs[job_idx].pgid;
	jobs[job_idx].state = JOB_RUNNING;

	kill(-pgid, SIGCONT);
	fprintf(stderr, "[%d] %s &\n", pgid, jobs[job_idx].cmdline);

	return 0;
}

int builtin_cd(char **args)
{
	char *dir;

	if (args[1] == NULL)
	{
		dir = getenv("HOME");
		if (dir == NULL)
		{
			fprintf(stderr, "cd: HOME not set\n");
			return -1;
		}
	}
	else
	{
		dir = args[1];
	}

	if (chdir(dir) != 0)
	{
		perror("cd");
		return -1;
	}

	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		setenv("PWD", cwd, 1);
	}

	return 0;
}

int builtin_jobs(void)
{
	extern struct job jobs[];
	extern int njobs;

	for (int i = 0; i < njobs; i++)
	{
		const char *state_str;
		switch (jobs[i].state)
		{
		case JOB_RUNNING:
			state_str = "Running";
			break;
		case JOB_STOPPED:
			state_str = "Stopped";
			break;
		case JOB_DONE:
			state_str = "Done";
			break;
		default:
			state_str = "Unknown";
		}
		fprintf(stderr, "[%d] %s\t%s\n", jobs[i].pgid, state_str, jobs[i].cmdline);
	}

	return 0;
}

int builtin_exit(void)
{
	cleanup_jobs();
	exit(0);
}

int execute_builtin(char *cmd, char **args)
{
	if (strcmp(cmd, "fg") == 0)
	{
		return builtin_fg();
	}
	else if (strcmp(cmd, "bg") == 0)
	{
		return builtin_bg();
	}
	else if (strcmp(cmd, "cd") == 0)
	{
		return builtin_cd(args);
	}
	else if (strcmp(cmd, "jobs") == 0)
	{
		return builtin_jobs();
	}
	else if (strcmp(cmd, "exit") == 0)
	{
		return builtin_exit();
	}

	return -1;
}

int is_builtin(char *cmd)
{
	return (strcmp(cmd, "fg") == 0 ||
			strcmp(cmd, "bg") == 0 ||
			strcmp(cmd, "cd") == 0 ||
			strcmp(cmd, "jobs") == 0 ||
			strcmp(cmd, "exit") == 0);
}
