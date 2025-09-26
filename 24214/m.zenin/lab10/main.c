#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv){
	if (argc < 2){
		fprintf(stderr, "No command to execute is provided\n");
		return -1;
	}

	pid_t child_pid = fork();

	if (child_pid == -1){
		perror("Failed to create new process");
		return -1;
	}

	// Child
	if (child_pid == 0){
		char *file = argv[1];
		char *last_slash = strrchr(argv[1], '/');

		if (last_slash){
			argv[1] = last_slash + 1;
		}

		execvp(file, argv + 1);

		perror("Failed to execute program");
		return -1;
	}

	// Parent
	int child_stat;
	if (waitpid(child_pid, &child_stat, 0) == -1){
		perror("Wait failed");
		return -1;
	}

	if (!WIFEXITED(child_stat)){
		printf("%s doesn't terminated normally\n", argv[1]);
		return 0;
	}

	printf("Return code of %s is %d\n", argv[1], (int) WEXITSTATUS(child_stat));

	return 0;
}

