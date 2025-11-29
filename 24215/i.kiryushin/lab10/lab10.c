#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    if (argc < 2)
    {
        fprintf(stderr, "Not enough arguments\n");
        exit(EXIT_FAILURE);
    } 
    
    pid_t pid;
    pid = fork();
    int status;

    switch (pid) {
        case (-1):
            perror("Error fork");
            exit(EXIT_FAILURE);
        case (0):
            if (execvp(argv[1], &argv[1]) == -1) {
                perror("Error execvp");
                exit(EXIT_FAILURE);
            }
        default:
            if (waitpid(pid, &status, 0) == -1){
                perror("Error waitpid");
                exit(EXIT_FAILURE);
            }
            if (WIFEXITED(status)) {
                printf("proccess is over, status is %d\n", WEXITSTATUS(status));
            }       
            
    }

    exit(EXIT_SUCCESS);
}
