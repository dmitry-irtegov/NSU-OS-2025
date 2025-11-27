#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    } 
    else if (pid == 0) {
        execlp("cat", "cat", "bigfile.txt", NULL);
        perror("execlp");
        exit(1);
    } 
    else {
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
