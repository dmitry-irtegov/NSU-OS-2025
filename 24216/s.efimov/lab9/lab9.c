#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(){
    pid_t pid = fork();
    switch (pid) {
        case -1:
            perror("fork fail");
            exit(EXIT_FAILURE);
            
        case 0:
            execl("/bin/cat", "cat", "1.txt", NULL);
            perror("execl fail"); 
            exit(EXIT_FAILURE);
            
        default:
            if (waitpid(pid, NULL, 0) == -1) {
                perror("waitpid fail");
                exit(EXIT_FAILURE);
            }
            printf("\nI am parent\n");
    }
    
    return 0;
}