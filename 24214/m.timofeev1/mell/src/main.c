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
		if (is_builtin(cmds[0].cmdargs[0]))
		{
			execute_builtin(cmds[0].cmdargs[0], cmds[0].cmdargs);
			continue;
		}

		execute_pipeline(ncmds, line);
	}

	if (has_running_jobs())
	{
		wait_for_jobs();
	}

	return 0;
}
