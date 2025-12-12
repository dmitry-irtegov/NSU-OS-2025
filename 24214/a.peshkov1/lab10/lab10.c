#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    int status;
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        if (execvp(argv[1], &argv[1]) == -1) {
            perror("execvp failed");
            return 1;
        }
    } else {
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            return 1;
        }
        if (WIFEXITED(status)) {
            printf("Child exited with code: %d\n", WEXITSTATUS(status));
        } else {
            printf("Child terminated with no exit code.\n");
        }
    }

    return 0;
}
