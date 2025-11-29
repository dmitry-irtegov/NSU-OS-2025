#include "io.h"
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
        if (tcsetpgrp(0, getpgrp())) {
            perror("Could not set foreground process group");
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

pid_t launch_pipeline(pipeline_t *pipeline) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Could not fork a new process");
        return -1;
    }
    if (pid == 0) {
        setup_and_exec(pipeline);
        exit(1);
    }
    if (pipeline->flags & PLBKGRND) {
        fdprintf(1, "Launched BG job. pid: %ld\n", (long)pid);
    }

    // There is an unavoidable (without IPC) race condition:
    // - If we call setpgid in just the child process, the parent may beat the child 
    // and try to wait before the child calls setpgid, resulting in an error.
    // - If we call setpgid in just the parent process, the child may beat the parent 
    // and call exec. The call to setpgid in the parent process would then result in an error.
    // 
    // This is why we call setpgid in both processes.
    //
    // We ignore the result of this call because it may fail if the child manages to beat the parent.
    // Any other errors should be handled by the child.
    setpgid(pid, pid);
    return pid;
}
