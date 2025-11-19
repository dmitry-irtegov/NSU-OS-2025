#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    }

    if (pid == 0) {
        execlp("cat", "cat", "file.txt", NULL);
        perror("execlp");
        exit(1);
    }
    else {
        int status;
        waitpid(pid, &status, 0);
        printf("\nThis message appears after the ending of parent process\n");
    }
    return 0;
}