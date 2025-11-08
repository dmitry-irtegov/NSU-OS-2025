#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>

main(int argc, char *argv[]){
        int status;

        if (fork() == 0){
                execvp(argv[1], &argv[1]);
                perror(argv[1]);
                exit(127);
        }

        wait(&status);

        printf("\nexit status: %d\n", WEXITSTATUS(status));
        exit(0);
}

