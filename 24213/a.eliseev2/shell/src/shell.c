#include "io.h"
#include "parse.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[]) {

    /* PLACE SIGNAL CODE HERE */

    char is_eof = 0, is_error = 0;
    char line[RD_BUF_SIZE];
    
    struct termios new_attr;
    struct termios old_attr;
    if (setup_terminal(0, &old_attr, &new_attr)) {
        is_error = 1;
    }

    while (!is_error && !is_eof && prompt_line(line, sizeof(line), &is_eof)) {
        static pipeline_t pipeline;

        char *ptr = line;
        while (!is_error && parse_pipeline(&ptr, &pipeline)) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("Could not fork a new process");
                is_error = 1;
                break;
            }
            if (pid == 0) {
                if (pipeline.in_file) {
                    int fd = open(pipeline.in_file, O_RDONLY);
                    if (fd == -1) {
                        perror("Could not open file for reading");
                        return 1;
                    }
                    if (dup2(fd, 0) == -1) {
                        perror("Could not set stdin");
                        return 1;
                    }
                }
                if (pipeline.out_file) {
                    int flags = O_WRONLY | O_CREAT;
                    flags |= pipeline.flags & PLAPPEND ? O_APPEND : O_TRUNC;
                    int fd = open(pipeline.out_file, flags, 0666);
                    if (fd == -1) {
                        perror("Could not open file for writing");
                        return 1;
                    }
                    if (dup2(fd, 1) == -1) {
                        perror("Could not set stdout");
                        return 1;
                    }
                }
                execvp(pipeline.commands[0].args[0], pipeline.commands[0].args);
                perror("Could not execute");
                return 1;
            }

            if (pipeline.flags & PLBKGRND) {
                fdprintf(1, "Launched BG job. pid: %ld\n", (long)pid);
                continue;
            }

            siginfo_t info;
            if (waitid(P_PID, pid, &info, WEXITED)) {
                perror("Could not wait for child to exit");
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

/* PLACE SIGNAL CODE HERE */
