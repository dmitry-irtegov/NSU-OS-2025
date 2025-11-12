#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "shell.h"

// 13-14
void handler_child(int pid) {
    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return;
    }

#ifdef DEBUG
    printf("Debug: handler_child called for PID %d\n", pid);

    if (WIFEXITED(status)) {
        printf("Process %d exited with status %d\n", pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Process %d killed by signal %d\n", pid, WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
        printf("Process %d stopped by signal %d\n", pid, WSTOPSIG(status));
    }
#endif
}

// 15
static char* file_Error[3] = {"Error opening input file",
                              "Error opening output file",
                              "Error opening append file"};
/* type: 0 - infile, 1 - outfile, 2 - appfile */
void file_operation(char* name, int type) {
    int fd;
    char* error_msg = file_Error[type];
    switch (type) {
        case 0:
            fd = open(name, O_RDONLY);
            break;
        case 1:
            fd = open(name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            break;
        case 2:
            fd = open(name, O_CREAT | O_WRONLY | O_APPEND, 0644);
            break;
        default:
            perror("Invalid file operation type");
            exit(1);
    }
    if (fd == -1) {
        perror(error_msg);
        exit(1);
    }
    if (type == 0) {
        dup2(fd, STDIN_FILENO);
    } else {
        dup2(fd, STDOUT_FILENO);
    }
    close(fd);
}

void cleanup_zombies() {
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
#ifdef DEBUG
        printf("Background process PID: %d finished\n", pid);
#endif
    }
}