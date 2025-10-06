#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <wait.h>
#include <signal.h>

static void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            fprintf(stderr, "[bg done] pid=%d exit=%d\n", pid, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)) {
            fprintf(stderr, "[bg killed] pid=%d signal=%d\n", pid, WTERMSIG(status));
        }
    }
}