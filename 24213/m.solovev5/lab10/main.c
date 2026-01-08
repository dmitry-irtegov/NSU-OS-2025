#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Not enough arguments: provide a program name\n");
		exit(1);
	}

	pid_t pid = fork();

	if (pid == -1) {
		perror("fork failed");
		exit(1);
	} else if (pid == 0) {
		execvp(argv[1], argv + 1);
		perror("exec failed");
		exit(1);
	}

	int stat_loc;
	if (wait(&stat_loc) == -1) {
		perror("wait");
		exit(1);
	}

	if (WIFEXITED(stat_loc)) {
		printf("Child finished with code %d\n", WEXITSTATUS(stat_loc));
	} else if (WIFSIGNALED(stat_loc)) {
		printf("Child finished with signal %d\n", WTERMSIG(stat_loc));
	} else {
		fprintf(stderr, "Cannot determine child status code or signal\n");
		exit(1);
	}

	exit(0);
}
