#include "builtins.h"
#include "io.h"
#include "jobs.h"
#include "parse.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[]) {

    if (check_terminal(0)) {
        return 1;
    }

    static joblist_t jobs;
    char *line = NULL;
    char is_error = 0;
    
    signal(SIGINT, SIG_IGN);  /* To ignore ^C */
    signal(SIGQUIT, SIG_IGN); /* To ignore ^\ */
    signal(SIGTSTP, SIG_IGN); /* To ignore ^Z */
    signal(SIGTTOU, SIG_IGN); /* To call tcsetpgrp from background */
    
    prompt_init();
    init_jobs(&jobs);
    setbuf(stdout, NULL);

    while (!is_error && (line = prompt_line(line))) {
        parser_t parser = make_parser(line);
        static pipeline_t pipeline;

        while (!is_error && parse_pipeline(&parser, &pipeline)) {
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

        if (!is_error && wait_background(&jobs)) {
            is_error = 1;
            continue;
        }
    }

    free(line);

    return is_error;
}
