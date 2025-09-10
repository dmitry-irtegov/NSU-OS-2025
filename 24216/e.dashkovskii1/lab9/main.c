#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();
    switch (pid) {
    case -1:
        perror("erorr fork");
        return 1;
    case 0:
        execlp("cat", "cat", "file.txt", NULL);
        perror("error execlp");
        return 1;
    default:
        waitpid(pid, NULL, 0);
        printf("\nparent process\n");
        break;
    }
    return 0;
}