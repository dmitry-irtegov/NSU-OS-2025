#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256

int main()
{
	// Array for pipe descriptors, [0] - read, [1] - write
	int pipefd[2];
	pid_t pid;
	char buffer[BUFFER_SIZE];

	// Test message
	const char *message = "Hello, World! This is a TeSt.\n";

	if (pipe(pipefd) == -1)
	{
		perror("failed to create pipe");
		return -1;
	}

	pid = fork();
	if (pid == -1)
	{
		perror("failed to fork");
		return -1;
	}

	// Parent process
	if (pid > 0)
	{
		close(pipefd[0]);
		write(pipefd[1], message, strlen(message) + 1);
		close(pipefd[1]);

		wait(NULL);
	}
	// Child process
	else
	{
		close(pipefd[1]);

		ssize_t nread = read(pipefd[0], buffer, BUFFER_SIZE);
		if (nread == -1)
		{
			perror("failed to read from pipe");
			return -1;
		}

		for (int i = 0; i < nread; i++)
		{
			buffer[i] = toupper(buffer[i]);
		}

		printf("%s\n", buffer);

		close(pipefd[0]);
		return 0;
	}

	return 0;
}
