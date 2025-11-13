#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    switch(pid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0:
            execlp("cat", "cat", "test.txt", NULL);
            perror("execlp failed");
            exit(1);
        default:
            switch (waitpid(pid, NULL, 0)) {
                case -1:
                    perror("waitpid failed");
                    exit(1);
                default:
                    printf("\npreproccess finished successfully!\n");
            }
    }

    return 0;
}
