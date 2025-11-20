#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int status;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(1);
    }

    pid_t pid = fork();

    switch (pid) {
        case 0:
            execvp(argv[1], &argv[1]);
            perror("Error execvp");
            exit(1);

        case -1:
            perror("Error fork");
            exit(1);

        default:
            if (waitpid(pid, &status, 0) == -1) {
                perror("Error waitpid");
                exit(1);
            }

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            } else {
                return 1;
            }
    }

    return 0;
}