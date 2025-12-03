#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

int execvpe(const char* file, char* const argv[], char* const envp[]) {
    char** oldEnv = environ;
    environ = (char**)envp;
    execvp(file, argv);
    environ = oldEnv;
    return -1;
}

int main() {
    char* envp[] = {"PATH=/bin:/usr/bin", "TESTED=HelloGophers", NULL};
    char* argv[] = {"env", NULL};
    int status;
    
    pid_t pid = fork();
    switch (pid) {
        case 0:
            execvpe("env", argv, envp);
            perror("execvp error");
            exit(EXIT_FAILURE);

        case -1:
            perror("fork error");
            exit(EXIT_FAILURE);

        default:
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid error");
                exit(EXIT_FAILURE);
            }

            if (WIFEXITED(status)) {
                exit(WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                exit(128 + WTERMSIG(status));
            } else {
                exit(EXIT_FAILURE);
            }
    }
    exit(EXIT_SUCCESS);
}
