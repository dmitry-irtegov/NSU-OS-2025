#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please, provide the command to be run\n");
        return -1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        printf("Could not fork the process\n");
        return 1;
    }
    if (pid == 0) {
        execvp(argv[1], argv + 1);
        perror("Could not execute the command");
    } else {
        int status;
        wait(&status);
        printf("Execution %s with exit status %d\n",
                WEXITSTATUS(status) ? "failed" : "successful", WEXITSTATUS(status));
    }
    return 0;
}
