#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

int main(void) {
    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork failed");
            exit(EXIT_FAILURE);

        case 0:
            execlp("cat", "cat", "too_large_file", (char *)0);
            perror("execlp failed");
            exit(EXIT_FAILURE);

        default: {
            int child_status;
            pid_t wait_status = waitpid(pid, &child_status, 0);
            
            if (wait_status == -1) {
                perror("waitpid() failed");
                exit(EXIT_FAILURE);
            }

            printf("Parent process finished after child %d\n", pid);
            exit(EXIT_SUCCESS);
        }
    }
}
