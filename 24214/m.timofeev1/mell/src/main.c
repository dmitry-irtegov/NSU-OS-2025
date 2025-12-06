#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include "shell.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

extern struct job jobs[MAXJOBS];
extern int njobs;
pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

int main(int argc, char *argv[])
{
	char line[1024]; /*  allow large command lines  */
	int ncmds;
	char prompt[50];

	shell_terminal = STDIN_FILENO;
	shell_is_interactive = isatty(shell_terminal);

	if (shell_is_interactive)
	{
		shell_pgid = getpid();
		if (setpgid(shell_pgid, shell_pgid) < 0)
		{
			perror("setpgid");
			exit(1);
		}

		tcsetpgrp(shell_terminal, shell_pgid);
		tcgetattr(shell_terminal, &shell_tmodes);
	}

	init_signals();

	sprintf(prompt, "[%s] ", argv[0]);

	while (promptline(prompt, line, sizeof(line)) > 0)
	{
		cleanup_jobs();

		if ((ncmds = parseline(line)) <= 0)
			continue;

		int cmd_start = 0;
		for (int i = 0; i < ncmds; i++)
		{
			int is_last = (i == ncmds - 1);
			int is_pipeline_end = is_last || !(cmds[i].cmdflag & OUTPIP);

			if (is_pipeline_end)
			{
				int pipeline_len = i - cmd_start + 1;

				if (is_builtin(cmds[cmd_start].cmdargs[0]))
				{
					execute_builtin(cmds[cmd_start].cmdargs[0], cmds[cmd_start].cmdargs);
				}
				else
				{
					struct command saved_cmds[MAXCMDS];
					memcpy(saved_cmds, cmds, sizeof(cmds));

					for (int j = 0; j < pipeline_len; j++)
					{
						cmds[j] = saved_cmds[cmd_start + j];
					}

					execute_pipeline(pipeline_len, line);

					memcpy(cmds, saved_cmds, sizeof(cmds));
				}

				cmd_start = i + 1;
			}
		}
	}

	if (has_running_jobs())
	{
		wait_for_jobs();
	}

	return 0;
}
