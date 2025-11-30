#include "io.h"
#include "jobs.h"
#include "pipeline.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void setup_and_exec(pipeline_t *pipeline) {
    if (setpgid(0, 0)) {
        perror("Could not set PGID");
        return;
    }
    if (!(pipeline->flags & PLBKGRND)) {
        if (set_foreground(0, getpgrp())) {
            return;
        }
    }

    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);

    if (pipeline->in_file) {
        int fd = open(pipeline->in_file, O_RDONLY);
        if (fd == -1) {
            perror("Could not open file for reading");
            return;
        }
        if (dup2(fd, 0) == -1) {
            perror("Could not set stdin");
            return;
        }
    }
    if (pipeline->out_file) {
        int flags = O_WRONLY | O_CREAT;
        flags |= pipeline->flags & PLAPPEND ? O_APPEND : O_TRUNC;
        int fd = open(pipeline->out_file, flags, 0666);
        if (fd == -1) {
            perror("Could not open file for writing");
            return;
        }
        if (dup2(fd, 1) == -1) {
            perror("Could not set stdout");
            return;
        }
    }
    execvp(pipeline->commands[0].args[0], pipeline->commands[0].args);
    perror("Could not execute");
    return;
}

int launch_job(pipeline_t *pipeline, job_t *job) {
    struct termios term_attr;
    if (save_terminal(0, &term_attr)) {
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Could not fork a new process");
        return 1;
    }
    if (pid == 0) {
        setup_and_exec(pipeline);
        exit(1);
    }

    // There is an unavoidable (without IPC) race condition:
    // - If we call setpgid in just the child process, the parent may beat the
    // child and try to wait before the child calls setpgid, resulting in an
    // error.
    // - If we call setpgid in just the parent process, the child may beat the
    // parent and call exec. The call to setpgid in the parent process would
    // then result in an error.
    //
    // This is why we call setpgid in both processes.
    //
    // We ignore the result of this call because it may fail if the child
    // manages to beat the parent. Any other errors should be handled by the
    // child.
    setpgid(pid, pid);

    *job = (job_t){
        .procs[0] =
            (proc_t){
                .pid = pid,
                .state = PROC_RUNNING,
            },
        .count = 1,
        .pgid = pid,
        .state = PROC_RUNNING,
        .term_attr = term_attr,
    };
    return 0;
}
