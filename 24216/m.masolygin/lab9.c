#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>

// wait - very simple implementation
// waitid - more more more comlex implementation
// waitpid - normal complex implementation

int main()
{
    char *argv[] = {"/bin/cat", "file", (char *)0};
    int status;
    pid_t pid = fork();

    switch (pid)
    {
    case 0:
        execvp("/bin/cat", argv);
        perror("Error execvp");
        exit(1);

    case -1:
        perror("Error fork");
        exit(1);

    default:
        printf("Hello from parent!\n");

        pid_t waitpidStatus = waitpid(pid, &status, 0);
        if (waitpidStatus == -1)
        {
            perror("Error waitpid");
            exit(1);
        }

        if (WIFEXITED(status))
        {
            printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Child process %d was terminated by signal %d\n", pid, WTERMSIG(status));
        }
        else
        {
            printf("Child process %d ended dont normally\n", pid);
        }

        printf("\nChild process %d finished!\n", pid);
        break;
    }

    return 0;
}