#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include "shell.h"

void handler_child(int pid)
{
    int status;

    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid");
        return;
    }

#ifdef DEBUG
    printf("Debug: handler_child called for PID %d\n", pid);

    if (WIFEXITED(status))
    {
        printf("Process %d exited with status %d\n", pid, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        printf("Process %d killed by signal %d\n", pid, WTERMSIG(status));
    }
    else if (WIFSTOPPED(status))
    {
        printf("Process %d stopped by signal %d\n", pid, WSTOPSIG(status));
    }
#endif
}

void cleanup_zombies()
{
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
#ifdef DEBUG
        printf("Background process PID: %d finished\n", pid);
#endif
    }
}