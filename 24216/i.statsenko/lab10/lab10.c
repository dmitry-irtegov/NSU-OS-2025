#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "incorrect number of arguments\n");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    switch (pid) {
        case -1:
            perror("fork error");
            exit(EXIT_FAILURE);

        case 0:
            execvp(argv[1], &argv[1]);
            perror("exec error");
            exit(EXIT_FAILURE);

        default: { 
            int child_stat;
            if (waitpid(pid, &child_stat, 0) == -1) {
                perror("waitpid error");
                exit(EXIT_FAILURE);
            }

            if (!WIFEXITED(child_stat)) {
                printf("not normally terminated of process\n");
            } else {
                printf("child process exited with status: %d\n", WEXITSTATUS(child_stat));
            }
            exit(EXIT_SUCCESS);
        }
    }
}