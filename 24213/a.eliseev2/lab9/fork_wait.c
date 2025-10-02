#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <siginfo.h>

int main() {
    pid_t result = fork();

    if (result < 0) {
        perror("fork failed");
        return 1;
    } else if (result == 0) {
        char *const cat_args[] = {"cat", "/usr/share/man/man1/make.1", NULL};
        if (execvp(cat_args[0], cat_args) == -1) {
            perror("execv failed");
            return 1;
        }
    }

    printf("parent: Child launched!\n");
    fflush(stdout);

    siginfo_t info;
    if (waitid(P_PID, result, &info, WEXITED)) {
        perror("waitid failed");
        return 1;
    }

    printf("parent: Child exited!\n");
    fflush(stdout);

    return 0;
}
