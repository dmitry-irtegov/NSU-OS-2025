#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "shell.h"

extern char *infile, *outfile, *appfile;
void setup_io_redirection(int cmd_idx, int ncmds, int pipes[][2])
{
	int fd;

	if (cmd_idx == 0 && infile != NULL)
	{
		fd = open(infile, O_RDONLY);
		if (fd < 0)
		{
			perror(infile);
			exit(1);
		}
		dup2(fd, STDIN_FILENO);
		close(fd);
	}
	else if (cmd_idx > 0 && (cmds[cmd_idx].cmdflag & INPIP))
	{
		dup2(pipes[cmd_idx - 1][0], STDIN_FILENO);
	}

	if (cmd_idx == ncmds - 1)
	{
		if (outfile != NULL)
		{
			fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd < 0)
			{
				perror(outfile);
				exit(1);
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		else if (appfile != NULL)
		{
			fd = open(appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd < 0)
			{
				perror(appfile);
				exit(1);
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
	}
	else if (cmds[cmd_idx].cmdflag & OUTPIP)
	{
		dup2(pipes[cmd_idx][1], STDOUT_FILENO);
	}
}

void close_all_pipes(int npipes, int pipes[][2])
{
	for (int i = 0; i < npipes; i++)
	{
		close(pipes[i][0]);
		close(pipes[i][1]);
	}
}

void execute_command(int cmd_idx)
{
	execvp(cmds[cmd_idx].cmdargs[0], cmds[cmd_idx].cmdargs);
	perror(cmds[cmd_idx].cmdargs[0]);
	exit(1);
}
