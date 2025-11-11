#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(){
    pid_t pid;
    pid = fork();
    int status;

    switch (pid) {
        case (-1):
            perror("Error fork");
            exit(EXIT_FAILURE);
        case (0):
            if (execlp("cat", "cat", "a.txt", NULL) == -1){
                perror("Error execlp()");
                exit(EXIT_FAILURE);
            }
        default:
            if (waitpid(pid, &status, 0) == -1){
                perror("Error waitpid");
                exit(EXIT_FAILURE);
            }
            printf("Working!\n");
            exit(EXIT_SUCCESS);
    }

    return 0;
}