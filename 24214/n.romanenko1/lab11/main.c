#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

extern char** environ;

int execvpe(const char *file, char *const argv[], char *const envp[])
{
    if (!file || !argv || !envp)
    {
        errno = EINVAL;
        return -1;
    }

    char **old_environ = environ;

    environ = (char **)envp;
    
    execvp(file, argv);
    
    environ = old_environ;
    
    return -1;
}

int main()
{
    char *args[] = {"ls", "-l", "-a", NULL};
    
    char *env1[] = {"PATH=/bin", NULL};
     
    char *env2[] = {"PATH=", NULL};

    pid_t pid = fork();

    if(pid == 0)
    {   
        execvpe("ls", args, env2);
        printf("execvpe returned with error: %s\n", strerror(errno));

        execvpe("ls", args, env1);
        
        exit(1);
    }

    else if(pid > 0)
    {
        printf("parental process is waiting for the daughter\n");

        int status;
        waitpid(pid, &status, 0); 
    }

    else
    {
        perror("fork failed");
        return 1;
    }
    
    return 0;
}