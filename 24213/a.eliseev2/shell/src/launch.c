#include "io.h"
#include "jobs.h"
#include "pipeline.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int try_close(int fd) {
    if (close(fd)) {
        perror("Could not close file");
        return 1;
    }
    return 0;
}

static void setup_and_exec(command_t *command, pid_t pgid, int fd_in,
                           int fd_out, char bg) {
    if (setpgid(0, pgid)) {
        perror("Could not set PGID");
        return;
    }
    if (!bg && pgid == 0) {
        if (set_foreground(0, getpgrp())) {
            return;
        }
    }

    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);

    if (dup2(fd_in, 0) == -1) {
        perror("Could not set stdin");
        return;
    }
    if (dup2(fd_out, 1) == -1) {
        perror("Could not set stdout");
        return;
    }
    if (fd_in > 2 && try_close(fd_in)) {
        return;
    }
    if (fd_out > 2 && try_close(fd_out)) {
        return;
    }
    execvp(command->args[0], command->args);
    perror("Could not execute");
    return;
}

int launch_job(pipeline_t *pipeline, job_t *job) {
    struct termios term_attr;
    if (save_terminal(0, &term_attr)) {
        return 1;
    }

    *job = (job_t){
        .count = pipeline->cmd_count,
        .state = PROC_RUNNING,
        .pgid = 0,
        .term_attr = term_attr,
    };

    int fd_prev = 0;

    // Try to open the pipeline input file
    if (pipeline->in_file) {
        fd_prev = open(pipeline->in_file, O_RDONLY);
        if (fd_prev == -1) {
            perror("Could not open file for reading");
            return 0;
        }
    }

    for (int i = 0; i < pipeline->cmd_count; i++) {
        int fd_in = fd_prev, fd_out = 1;

        // Open a pipe for all commands except the first one
        if (i < pipeline->cmd_count - 1) {
            int fd_pipe[2];
            if (pipe(fd_pipe)) {
                perror("Could not open pipe");
                return 1;
            }
            fd_out = fd_pipe[1];
            fd_prev = fd_pipe[0];
            // Try to open the pipeline output file for the last command
        } else if (i == pipeline->cmd_count - 1 && pipeline->out_file) {
            int flags = O_WRONLY | O_CREAT;
            flags |= pipeline->flags & PLAPPEND ? O_APPEND : O_TRUNC;
            fd_out = open(pipeline->out_file, flags, 0666);
            if (fd_out == -1) {
                perror("Could not open file for writing");
                return 0;
            }
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("Could not fork a new process");
            return 1;
        }
        if (pid == 0) {
            // Close the "previous" file. stdin/stdout
            // will be closed in setup_and_exec
            if (fd_prev != fd_in && try_close(fd_prev)) {
                exit(1);
            }
            setup_and_exec(&pipeline->commands[i], job->pgid, fd_in, fd_out,
                           pipeline->flags & PLBKGRND);
            exit(1);
        }

        // Close the stdin/stdout of the newly created process
        // The only unclosed fd should be fd_prev, unless we're processing
        // the last command, in which case it would be equal to fd_in.
        if (fd_in > 2 && try_close(fd_in)) {
            return 1;
        }
        if (fd_out > 2 && try_close(fd_out)) {
            return 1;
        }

        job->procs[i] = (proc_t){
            .pid = pid,
            .state = PROC_RUNNING,
        };
        if (job->pgid == 0) {
            job->pgid = pid;
        }

        // There is an unavoidable (without IPC) race condition:
        // - If we call setpgid in just the child process, the parent may beat
        // the child and try to wait before the child calls setpgid, resulting
        // in an error.
        // - If we call setpgid in just the parent process, the child may beat
        // the parent and call exec. The call to setpgid in the parent process
        // would then result in an error.
        //
        // This is why we call setpgid in both processes.
        //
        // We ignore the result of this call because it may fail if the child
        // manages to beat the parent. 
        // Any other errors should be handled by the child.
        setpgid(pid, job->pgid);
    }

    return 0;
}
