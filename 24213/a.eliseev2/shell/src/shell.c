#include "io.h"
#include "pipeline.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[]) {

    char is_eof = 0, is_error = 0;
    char line[RD_BUF_SIZE];

    signal(SIGINT, SIG_IGN);  /* To ignore ^C */
    signal(SIGQUIT, SIG_IGN); /* To ignore ^\ */
    signal(SIGTSTP, SIG_IGN); /* To ignore ^Z */
    signal(SIGTTOU, SIG_IGN); /* To call tcsetpgrp from background */

    struct termios new_attr;
    struct termios old_attr;
    if (setup_terminal(0, &old_attr, &new_attr)) {
        is_error = 1;
    }

    while (!is_error && !is_eof && prompt_line(line, sizeof(line), &is_eof)) {
        static pipeline_t pipeline;

        char *ptr = line;
        while (!is_error && parse_pipeline(&ptr, &pipeline)) {
            pid_t pgid = launch_pipeline(&pipeline);

            if (pgid == -1) {
                is_error = 1;
                break;
            }

            if (pipeline.flags & PLBKGRND) {
                continue;
            }

            siginfo_t info;
            // We'll handle WSTOPPED properly when jobs are implemented
            if (waitid(P_PGID, pgid, &info, WEXITED | WSTOPPED)) {
                perror("Could not wait for child");
                is_error = 1;
                break;
            }
            if (tcsetpgrp(0, getpgrp())) {
                perror("Could not set foreground process group");
                is_error = 1;
                break;
            }
            if (restore_terminal(0, &new_attr)) {
                is_error = 1;
                break;
            }
        }
    }

    if (restore_terminal(0, &old_attr)) {
        is_error = 1;
    }
    return !is_eof || is_error;
}
