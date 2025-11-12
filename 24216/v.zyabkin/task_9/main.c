#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        execlp("cat", "cat", "test.txt", NULL);
        perror("execlp failed");
        exit(1);
    } else {
        if (waitpid(pid, NULL, 0) != -1) {
            printf("\npreproccess finished successfully!\n");
        } else {
            perror("waitpid failed");
            exit(1);
        }
    }
    return 0;
}
