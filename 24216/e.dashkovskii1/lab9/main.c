#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();
    
    switch (pid) {
    case -1:
        perror("erorr fork");
        exit(1);
    case 0:
        execlp("cat", "cat", "file.txt", NULL);
        perror("error execlp");
        exit(1);
    default:
        if (waitpid(pid, NULL, 0) == -1) {
            perror("error waitpid");
            exit(1);
        }
        printf("\nparent process\n");
        break;
    }
    exit(0);
}