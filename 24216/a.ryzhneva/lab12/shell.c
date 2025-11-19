#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include "jobs.h"
#include "shell.h"
#include "builtins.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
struct termios shell_termios;
char bkgrnd;

const int signals[] = { SIGINT, SIGTTIN, SIGTSTP, SIGQUIT, SIGTTOU };
const int count_signals = sizeof(signals) / sizeof(int);

void set_child_signals() {
    signal(SIGCHLD, SIG_DFL);
    for (int i = 0; i < count_signals; i++) {
        signal(signals[i], SIG_DFL);
    }
}

void build_job_prompt(int start_cmd, int end_cmd, char* prompt_buffer, size_t buflen) {
    *prompt_buffer = '\0';

    for (int k = start_cmd; k <= end_cmd; k++) {
        for (char** arg = cmds[k].cmdargs; *arg; arg++) {
            size_t left = buflen - strlen(prompt_buffer) - 1;
            strncat(prompt_buffer, *arg, left);

            if (*(arg + 1) != NULL) {
                left = buflen - strlen(prompt_buffer) - 1;
                strncat(prompt_buffer, " ", left);
            }
        }
        if (k < end_cmd) {
            size_t left = buflen - strlen(prompt_buffer) - 1;
            strncat(prompt_buffer, " | ", left);
        }
    }
}

int main(int argc, char *argv[]) { 

    for (int i = 0; i < count_signals; i++) {
        signal(signals[i], SIG_IGN);
    }

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "shell not interractive");
        exit(1);
    }

    if (tcgetattr(STDIN_FILENO, &shell_termios) != 0) {
        perror("tcgetattr failed");
        exit(EXIT_FAILURE);
    }

    if (tcsetpgrp(0, getpgrp()) == -1) {
        perror("tcsetpgrp failed");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    int ncmds = 0;
    char prompt[50];

    sprintf(prompt, "[%s] ", argv[0]);

    while (1) {
        
        check_update_states();

        if (promptline(prompt, line, sizeof(line)) <= 0) {
            break;
        }

        if ((ncmds = parseline(line)) <= 0) {
            continue; 
        }

        for (int i = 0; i < ncmds; i++) {
            if (built(cmds[i].cmdargs)) {
                continue;
            }
            
            int j = i;
            while (j < ncmds - 1 && (cmds[j].cmdflag & OUTPIP)) {
                j++;
            }
            
            pid_t pgid = 0;
            Proc* proc_list_head = NULL;
            Proc* proc_list_tail = NULL;
            int pipefd[2];
            int prev_pipe_read_end = -1;
            char job_prompt[1024];

            build_job_prompt(i, j, job_prompt, sizeof(job_prompt));

            for (int k = i; k <= j; k++) {
                if (k < j) {
                    if (pipe(pipefd) < 0) {
                        perror("pipe");
                        break;
                    }
                }

                pid_t pid = fork();

                switch (pid) {
                    case -1:
                        perror("fork");
                        exit(1);
                    case 0:
                        set_child_signals();

                        if (pgid == 0) {
                            pgid = getpid();
                        }

                        if (setpgid(pid, pgid) == -1) {
                            fprintf(stderr, "setpgid(%d, %d) failed: %s\n", pid, pgid, strerror(errno));
                        }

                        if (k > i) {
                            if (dup2(prev_pipe_read_end, STDIN_FILENO) < 0) {
                                perror("dup2 (stdin pipe) failed");
                                exit(1);
                            }
                            close(prev_pipe_read_end);
                        }
                        if (k < j) {
                            if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                                perror("dup2 (stdout pipe) failed");
                                exit(1);
                            }
                            close(pipefd[0]);
                            close(pipefd[1]);
                        }

                        if (infile != NULL && k == i) {
                            int fd = open(infile, O_RDONLY);
                            if (fd < 0) {
                                perror("infile");
                                exit(1);
                            }

                            if (dup2(fd, STDIN_FILENO) < 0) {
                                perror("dup2 (infile) failed");
                                exit(1);
                            }
                            if (close(fd) < 0) {
                                perror("close");
                            }
                        }

                        if ((outfile != NULL || appfile != NULL) && k == j) {
                            int fd;
                            if (appfile != NULL) {
                                fd = open(appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                            } 
                            else {
                                fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            }

                            if (fd < 0) {
                                perror(outfile ? outfile : appfile);
                                exit(1);
                            }

                            if (dup2(fd, STDOUT_FILENO) < 0) {
                                perror("dup2 (outfile) failed");
                                exit(1);
                            }
                            close(fd);
                        }

                        execvp(cmds[k].cmdargs[0], cmds[k].cmdargs);
                        perror(cmds[k].cmdargs[0]);
                        exit(127);
                    default:
                        if (pgid == 0) {
                            pgid = pid;
                        }
                        if (setpgid(pid, pgid) == -1) {
                            fprintf(stderr, "setpgid(%d, %d) failed: %s\n", pid, pgid, strerror(errno));
                        }
                        
                        char proc_prompt[256] = "";

                        for (char** arg = cmds[k].cmdargs; *arg; arg++) {
                            strcat(proc_prompt, *arg);
                            strcat(proc_prompt, " ");
                        }

                        Proc* proc = create_proc(pid, proc_prompt);
                        if (proc_list_head == NULL) {
                            proc_list_head = proc_list_tail = proc;
                        } else {
                            proc_list_tail->next = proc;
                            proc_list_tail = proc;
                        }

                        if (prev_pipe_read_end != -1) {
                            close(prev_pipe_read_end);
                        }
                        if (k < j) {
                            prev_pipe_read_end = pipefd[0];
                            close(pipefd[1]);
                        }
                }
            }   

            Job* job = create_job(pgid, RUNNING, job_prompt, proc_list_head);

            if (bkgrnd) {
                job->fg = 0;
                fprintf(stderr, "[%d] %d\n", job->number, job->pgid);
            } else {
                job->fg = 1;

                if (tcsetpgrp(0, job->pgid) == -1) {
                    perror("tcsetpgrp failed for child");
                }

                waiting_fg(job);

                if (tcsetpgrp(0, getpgrp()) == -1) {
                    perror("tcsetpgrp failed");
                }

                if (tcsetattr(0, TCSADRAIN, &shell_termios) != 0) {
                    perror("tcsetattr() failed");
                }
            }
            i = j;
        }
    }
    kill_job();
    tcsetattr(0, TCSADRAIN, &shell_termios);
    printf("\n");
    return 0;
}