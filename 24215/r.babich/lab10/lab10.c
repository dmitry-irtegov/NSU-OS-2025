#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "Argc < 2.\n");
		return 1;
	} 
	pid_t pid = fork();
	int status;

	switch (pid) {
		case -1:
			perror("Fork failed.");
			return 1;
		case 0:
			if (execvp(argv[1], &argv[1]) == -1) {
				perror("Execvp failed.");
				return 1;
			}
		default:
			if (waitpid(pid, &status, 0) == -1) {
      	perror("Waitpid failed.");
      	return 1;
    	}
			if (!WIFEXITED(status)) {
				fprintf(stderr, "Child process did not exit normally.\n");
			} else {
				printf("Child process exited with code %d\n", WEXITSTATUS(status));
			}
	}
	return 0;
} 
