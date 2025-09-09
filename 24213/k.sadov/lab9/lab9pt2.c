#include <stdio.h>
#include <stdlib.h>   
#include <unistd.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(){
    pid_t pid = fork();
    int status;

    if (pid == -1){
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0){

        execlp("cat", "cat", "long_file.txt", NULL);
        perror("execlp failed");
        exit(EXIT_FAILURE);
    }else{

         waitpid(pid, &status, 0);
         printf("Родительский процесс выводит этот текст ПОСЛЕ завершения дочернего\n");
    }
    return 0;
}