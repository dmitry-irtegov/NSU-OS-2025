#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "error: no input txt file\n");
        exit(EXIT_FAILURE);
    }

    pid_t childPID;
    pid_t wpid;
    int status;

    childPID = fork();
    if (childPID == -1)
    {
        perror("error: fork failed\n");
        exit(EXIT_FAILURE);
    }

    if (childPID > 0)
    {
        printf("[Parent] waiting for child pid: %d\n", childPID);

        wpid = wait(&status);
        if (wpid == -1)
        {
            perror("error: wait failed\n");
            exit(EXIT_FAILURE);
        }

        printf("[Parent] child with pid=%d finished\n", wpid);
    }
    else if (childPID == 0)
    {
        printf("[Child] started with pid: %d\n", getpid());
		
        if (execlp("cat", "cat", argv[1], (char*)NULL) == -1)
        {
            perror("error: cat failed\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
