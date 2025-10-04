#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		perror("Not enough arguments");
		exit(1);
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		perror("fork failed");
		exit(1);
	}

	if (pid == 0)
	{
		if (execvp(argv[1], &argv[1]) == -1)
		{
			perror("execvp failed");
			exit(1);
		}
	}
	else
	{
		// Parent process
		int status;
		waitpid(pid, &status, 0);

		if (WIFEXITED(status))
		{
			printf("Exit code: %d\n", WEXITSTATUS(status));
		}
		else
		{
			printf("Process terminated abnormally\n");
		}
	}

	return 0;
}
