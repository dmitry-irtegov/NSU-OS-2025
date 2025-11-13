#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "shell.h"

void handler_child(pid_t pgid, int ncmds, char* cmdline) {
    int status;

    for (int i = 0; i < ncmds; i++) {
        pid_t wpid = waitpid(-pgid, &status, WUNTRACED);

        if (wpid == -1) {
            perror("waitpid");
            continue;
        }

        if (WIFEXITED(status)) {
            delete_job(wpid);
#ifdef DEBUG
            printf("Process %d exited with status %d\n", wpid,
                   WEXITSTATUS(status));
#endif
        } else if (WIFSIGNALED(status)) {
            delete_job(wpid);
#ifdef DEBUG
            printf("Process %d killed by signal %d\n", wpid, WTERMSIG(status));
#endif
        } else if (WIFSTOPPED(status)) {
            struct job* jb = get_job_by_pid(wpid);
            if (jb == NULL) {
                add_job(wpid, pgid, STOPPED, cmdline);
            } else {
                jb->state = STOPPED;
                jb->pgid = pgid;
            }
            jb = get_job_by_pid(wpid);
            if (jb != NULL) {
                fprintf(stderr, "\n[%d]+ Stopped %d\n", jb->jid, jb->pid);
            }
#ifdef DEBUG
            printf("Process %d stopped by signal %d\n", wpid, WSTOPSIG(status));
#endif
            break;
        }
    }
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
        struct job* jb = get_job_by_pid(pid);
        if (jb != NULL) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                fprintf(stderr, "[%d] Done %d  %s\n", jb->jid, jb->pid,
                        jb->cmdline);
                delete_job(pid);
            }
        }
#ifdef DEBUG
        printf("Background process PID: %d finished\n", pid);
#endif
    }
}

void set_terminal_foreground(pid_t pgid) {
    if (isatty(STDIN_FILENO)) {
        if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
            perror("tcsetpgrp");
            exit(1);
        }
    }
}