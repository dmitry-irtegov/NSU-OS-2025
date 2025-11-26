#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

int execvpe(const char* file, char* const argv[], char* const envp[]) {
    char** origin_environ = environ;
    environ = (char**)envp;
    execvp(file, argv);
    environ = origin_environ;
    return -1;
};

int main() {
    int status;
    pid_t pid = fork();
    char* argv[] = {"sh", "-c", "echo MY_VAR=$MY_VAR", NULL};
    char* envp[] = {"MY_VAR=Hello_from_execvpe", "PATH=/bin:/usr/bin", NULL};
    switch (pid) {
        case 0:
            execvpe("sh", argv, envp);
            perror("Error execvp");
            exit(1);

        case -1:
            perror("Error fork");
            exit(1);

        default:
            if (waitpid(pid, &status, 0) == -1) {
                perror("Error waitpid");
                exit(1);
            }

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            } else {
                return 1;
            }
    }

    return 0;
}