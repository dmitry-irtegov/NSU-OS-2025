#include "builtins.h"
#include "io.h"
#include "jobs.h"
#include "pipeline.h"
#include "shell_limits.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[]) {


    signal(SIGINT, SIG_IGN);  /* To ignore ^C */
    signal(SIGQUIT, SIG_IGN); /* To ignore ^\ */
    signal(SIGTSTP, SIG_IGN); /* To ignore ^Z */
    signal(SIGTTOU, SIG_IGN); /* To call tcsetpgrp from background */

    if (check_terminal(0)) {
        return 1;
    }

    static joblist_t jobs;
    struct termios old_attr;
    char is_eof = 0, is_error = 0;
    char line[MAXLINE];

    if (setup_terminal(0, &old_attr)) {
        is_error = 1;
    }
    init_jobs(&jobs);
    setbuf(stdout, NULL);

    while (!is_error && !is_eof && prompt_line(line, sizeof(line), &is_eof)) {
        if (restore_terminal(0, &old_attr)) {
            is_error = 1;
            continue;
        }

        char *ptr = line;
        static pipeline_t pipeline;
        while (!is_error && parse_pipeline(&ptr, &pipeline)) {
            if (try_builtin(&pipeline, &jobs, &is_error)) {
                continue;
            }

            if (!can_add(&jobs)) {
                fprintf(stderr, "Too many running jobs!\n");
                continue;
            }

            job_t job;
            if (launch_job(&pipeline, &job)) {
                is_error = 1;
                continue;
            }
            add_job(&jobs, &job, pipeline.flags & PLBKGRND);
        }

        if (setup_terminal(0, &old_attr)) {
            is_error = 1;
            continue;
        }

        if (!is_error && wait_background(&jobs)) {
            is_error = 1;
            continue;
        }
    }

    if (restore_terminal(0, &old_attr)) {
        is_error = 1;
    }
    return !is_eof || is_error;
}
