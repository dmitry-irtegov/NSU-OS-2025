#include <stdio.h>
#include <stdlib.h>   
#include <unistd.h>   
#include <sys/wait.h> 
#include <sys/types.h>

int main(){
    pid_t pid = fork();

    if (pid == -1){
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0){

        execlp("cat", "cat", "long_file.txt", NULL);
        perror("execlp failed");
        exit(EXIT_FAILURE);
    }else{
 
        printf("Родительский процесс выводит этот текст\n");
    }
    return 0;
}
