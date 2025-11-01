#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    if (argc < 2){
		fprintf(stderr, "No command to execute is specified\n");
		return 1;
	}

    pid_t child_process;
    switch (child_process = fork()) {
    case -1:
        perror("Failed to create child process");
        return 1;
    case 0:
        execvp(argv[1], &argv[1]);
        perror("Failed to run command");
		return 1;
    default: {
         int child_status;
        if (waitpid(child_process, &child_status, 0) == -1) {
            perror("Failed waitpid");
            return 1;
        }

        if (!WIFEXITED(child_status)){
		    printf("The process did not terminate normally\n");
		    return 0;
	    }
	    printf("Process exit code: %d\n", (int) WEXITSTATUS(child_status));
        break;
    }
    }
    return 0;
}
