#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include "shell.h"

struct job jobs[MAXJOBS];
int njobs = 0;

extern pid_t shell_pgid;
extern struct termios shell_tmodes;
extern int shell_terminal;

int add_job(pid_t pgid, int state, char *cmdline)
{
	if (njobs >= MAXJOBS)
		return -1;
	jobs[njobs].pgid = pgid;
	jobs[njobs].state = state;
	jobs[njobs].cmdline = strdup(cmdline);
	return njobs++;
}

int find_job(pid_t pgid)
{
	for (int i = 0; i < njobs; i++)
	{
		if (jobs[i].pgid == pgid)
			return i;
	}
	return -1;
}

void cleanup_jobs()
{
	for (int i = 0; i < njobs; i++)
	{
		if (jobs[i].state == JOB_DONE)
		{
			free(jobs[i].cmdline);
			for (int j = i; j < njobs - 1; j++)
			{
				jobs[j] = jobs[j + 1];
			}
			njobs--;
			i--;
		}
	}
}

void sigchld_handler(int sig)
{
	pid_t pid;
	int status;
	int saved_errno = errno;

	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
	{
		pid_t pgid = getpgid(pid);
		int job_idx = find_job(pgid);

		if (job_idx >= 0)
		{
			if (WIFEXITED(status) || WIFSIGNALED(status))
			{
				jobs[job_idx].state = JOB_DONE;
			}
			else if (WIFSTOPPED(status))
			{
				jobs[job_idx].state = JOB_STOPPED;
			}
			else if (WIFCONTINUED(status))
			{
				jobs[job_idx].state = JOB_RUNNING;
			}
		}
	}

	errno = saved_errno;
}

void update_job_state(int job_idx, int state)
{
	if (job_idx >= 0 && job_idx < njobs)
	{
		jobs[job_idx].state = state;
	}
}

char *get_job_cmdline(int job_idx)
{
	if (job_idx >= 0 && job_idx < njobs)
	{
		return jobs[job_idx].cmdline;
	}
	return NULL;
}

int has_running_jobs()
{
	for (int i = 0; i < njobs; i++)
	{
		if (jobs[i].state == JOB_RUNNING)
		{
			return 1;
		}
	}
	return 0;
}

void wait_for_jobs()
{
	pid_t pid;
	int status;

	while (has_running_jobs())
	{
		pid = waitpid(-1, &status, WNOHANG);
		if (pid > 0)
		{
			int job_idx = find_job(pid);
			if (job_idx >= 0)
			{
				if (WIFEXITED(status) || WIFSIGNALED(status))
				{
					jobs[job_idx].state = JOB_DONE;
					cleanup_jobs();
				}
			}
		}
		else
		{
			usleep(100000);
		}
	}
}
