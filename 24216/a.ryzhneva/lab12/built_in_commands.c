#include <stdio.h>
#include "shell.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include "jobs.h"
#include <string.h>

extern struct termios shell_termios;

unsigned char built(char** args) {
    if (args[0] == NULL) {
        return 0;
    }

    if (strcmp("jobs", args[0]) == 0) { 
        print_all_jobs();
        return 1;
    }

    if (strcmp("bg", args[0]) == 0) {
        Job* job = parsing_job(args[1]);
        if (job) {
            switch_to_bg(job);
        }
        return 1;
    }

    if (strcmp("fg", args[0]) == 0) {
        Job* job = parsing_job(args[1]);
        if (job == NULL) {
            return 1;
        }

        job->fg = 1;
        if (tcsetpgrp(0, job->pgid) == -1) {
            perror("tcsetpgrp() failed");
            return 1;
        }

        if (kill(-job->pgid, SIGCONT) == -1) {
            perror("kill(SIGCONT) failed");
        }

        waiting_fg(job);

        if (tcsetpgrp(0, getpgrp()) == -1) {
            perror("tcsetpgrp() failed");
        }
        return 1;
    }

    if (strcmp("cd", args[0]) == 0) {
        if (args[1] == NULL) {
            args[1] = getenv("HOME");
            if (args[1] == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return 1;
            }
        }
        if (chdir(args[1]) == -1) {
            perror("cd failed");
        }
        return 1;
    }

    if (strcmp("exit", args[0]) == 0) {
        kill_job();
        tcsetattr(0, TCSADRAIN, &shell_termios);
        printf("\n");
        exit(0);
    }
    return 0;
}