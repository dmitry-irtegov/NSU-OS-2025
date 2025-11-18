#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "wrong count of arguments. command needed\n");
        exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork failed. could not fork the process");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        execvp(argv[1], argv + 1);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }

    int stat_loc;
    if (waitpid(child_pid, &stat_loc, 0) == -1) {
        perror("waitpid failed");
        exit(EXIT_FAILURE);
    }

    if (!WIFEXITED(stat_loc)) {
        fprintf(stderr,"the process did not terminate normally\n");
        exit(EXIT_FAILURE);
    }
    if (WEXITSTATUS(stat_loc) == 0) {
        printf("execution completed successfully with exit status %d\n", WEXITSTATUS(stat_loc));
    } else {
        printf("execution failed with exit status %d\n", WEXITSTATUS(stat_loc));
    }
    exit(EXIT_SUCCESS);
}

