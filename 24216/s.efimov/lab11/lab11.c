#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

extern char **environ;

int execvpe(const char *file, char *const argv[], char *const envp[]) {
    if (file == NULL || argv == NULL || envp == NULL) {
        errno = EINVAL;  
        return -1;
    }
    char **save = environ;
    environ = (char **)envp;
    execvp(file, argv);
    environ = save;
    return -1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }
    
    char *env[] = {
        "Course=second",
        "USER=student", 
        "PATH=/bin:/usr/bin",
        NULL
    };

    int status;
    pid_t pid = fork();
    
    switch (pid) {
        case 0:
            execvpe(argv[1], &argv[1], env);
            perror("execvpe failed");
            exit(1);

        case -1:
            perror("fork failed");
            return 1;

        default:
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else {
                return 1;
            }
    }

    return 0;
}