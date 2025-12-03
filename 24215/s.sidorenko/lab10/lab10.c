#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        perror("No arguments");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Error while creating process");
        return EXIT_FAILURE;
    }

    if (pid == 0) //доч проц
    {
        execvp(argv[1], &argv[1]);
        perror("Execvp error");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        int status;
        int res = waitpid(pid, &status, 0);

        if (res == -1)
        {
            perror("waitpid failed");
            return EXIT_FAILURE;
        }

        if (WIFEXITED(status))
        {
            printf("Process ended with exit code: %d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Process ended with signal %d\n", WTERMSIG(status));
        }
        else
        {
            printf("Process ended with something unexpected");
        }
    }
}