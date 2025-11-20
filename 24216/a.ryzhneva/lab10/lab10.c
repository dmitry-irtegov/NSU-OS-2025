#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("no enough args\n");
        return 1;
    }

    pid_t pid = fork();

    int status;
    switch (pid) {
        case -1:
            perror("fork failed");
            return 1;

        case 0:
            execvp(argv[1], &argv[1]);
            perror("execvp failed");
            return 1;

        default:
            if (wait(&status) == -1) { 
                perror("wait failed");
                exit(EXIT_FAILURE);
            }
        
        
            if (WIFEXITED(status)) {
                printf("success proc: %d\n", WEXITSTATUS(status));
            }

            else if (WIFSIGNALED(status)) {
                int signal_num = WTERMSIG(status);
                printf("proc sighneled: %d\n", signal_num);
            }    
            else {
                printf("proc finished\n");
            }
    }

    return 0;
}