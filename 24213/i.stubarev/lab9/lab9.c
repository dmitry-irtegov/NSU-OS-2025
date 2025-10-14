#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    pid_t pid;
    switch (pid=fork())
    {
    case -1:
        perror("Error fork");
        return 1;
    case 0:
        execl("/bin/cat", "cat", "text.txt", NULL);
        perror("Error execl");
        return 1;
    default:   
        if (waitpid(pid, NULL, 0) == -1) {
            perror("Error waitpid");
            return 1;
        }
        printf("i am the parent process\n");
        break;
    }
    return 0;
}
