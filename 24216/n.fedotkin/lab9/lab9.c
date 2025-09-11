#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();

    const char *filename = "test.txt";

    switch (pid) {
        case -1:
            perror("Fork failed");
            exit(EXIT_FAILURE);
        case 0:
            execlp("cat", "cat", filename, NULL);
            perror("execlp failed");
            exit(EXIT_FAILURE);
        default:
            int childStatus;
            pid_t waitedPid = waitpid(pid, &childStatus, 0);
            if (waitedPid == -1) {
                perror("waitpid failed");
                exit(EXIT_FAILURE);
            }
            printf("Parent: Child process %d\n", waitedPid);
            exit(EXIT_SUCCESS);
    }

}
