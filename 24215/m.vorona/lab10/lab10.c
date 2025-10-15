#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: No command to run!");
        exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(127);
    } else {
        printf("Spawned child process with PID = %d\n", pid);

        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(1);
        }

        if (WIFEXITED(status)) {
            printf("Child exited normally with code %d\n", WEXITSTATUS(status));
        } else {
            printf("Child exited abnormally");
        }
    }

    return 0;
}