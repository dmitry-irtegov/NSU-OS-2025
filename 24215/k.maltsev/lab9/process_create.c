#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <long_file>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execlp("cat", "cat", argv[1], (char *)NULL);
        perror("execlp");
        _exit(127);
    } 
    else {
        printf("Parent: started, child PID = %d\n", pid);
        printf("Parent: working while child is running...\n");

        int status = 0;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }

        if (WIFEXITED(status)) {
            printf("Parent: child exited with code %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Parent: child terminated by signal %d\n", WTERMSIG(status));
        } else {
            printf("Parent: child terminated abnormally\n");
        }

        printf("Parent: last line (printed after child finishes)\n");
    }
    return 0;
}