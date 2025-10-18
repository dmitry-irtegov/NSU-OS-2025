#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Please, provide the command to be run\n");
        exit(-1);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("Could not fork the process");
        exit(-1);
    }
    if (pid == 0) {
        execvp(argv[1], argv + 1);
        perror("Could not execute the command");
    } else {
        int status;
        wait(&status);
        printf("Execution %s with exit status %d\n",
                WEXITSTATUS(status) ? "failed" : "completed", WEXITSTATUS(status));
    }
    exit(0);
}
