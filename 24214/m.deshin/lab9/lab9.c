#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork failed");
            return 1;
        case 0:
            execlp("cat", "cat", "file.txt", NULL);
            perror("execlp failed");
            return 1;
        default:
            if (waitpid(pid, NULL, 0) == -1) {
                perror("waitpid failed");
                return 1;
            }
            printf("\nPARENT PROCESS\n");
    }

    return 0;
}