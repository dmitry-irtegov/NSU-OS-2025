#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();
    pid_t status;

    switch (pid) {
        case (-1):
            perror("Fork failed.");
            return 1;
        case (0):
            execl("/usr/bin/cat", "cat", "bigFile.txt", NULL);
            perror("Execl failed.");
            return 1;
        default:
            printf("\nparrent 1\n");
            if (waitpid(pid, &status, WUNTRACED) == -1) {
                perror("Waitpid failed.");
                return 1;
            }
            printf("\nparrent 2\n");
            return 0;
        }
}
