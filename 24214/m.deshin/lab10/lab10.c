#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Too little arguments!\nUsage: %s <command> [arg0...argn]\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    switch (pid) {
        case -1:
            perror("fork failed");
            return 1;
        case 0:
            execvp(argv[1], &argv[1]);
            perror("execvp failed");
            return 1;
        default: {
            int wstatus;

            if (waitpid(pid, &wstatus, 0) == -1) {
                perror("waitpid failed");
                return 1;
            }

            if (!WIFEXITED(wstatus)) {
                fprintf(stderr, "child process didn't terminate normally\n");
            } else {
                printf("Exit status of the child process: %d\n", WEXITSTATUS(wstatus));
            }
            return 0;
        }
    }
}
