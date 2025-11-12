#include "shell.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char* argv[]) {
    register int i;
    char line[MAXLINE]; /*  allow large command lines  */
    int ncmds;
    char prompt[50]; /* shell prompt */
    pid_t child_pid = -1;

    /* PLACE SIGNAL CODE HERE */
    ignore_signals();

    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("setpgid");
        exit(1);
    }

    if (isatty(STDIN_FILENO)) {
        if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
            perror("tcsetpgrp");
            exit(1);
        }
    }

    init_jobs();

    sprintf(prompt, "[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) { /* until eof  */

        cleanup_zombies();

        char original_line[MAXLINE];
        strncpy(original_line, line, MAXLINE - 1);
        original_line[MAXLINE - 1] = '\0';

        if ((ncmds = parseline(line)) <= 0) continue; /* read next line */
#ifdef DEBUG
        {
            int i, j;
            for (i = 0; i < ncmds; i++) {
                for (j = 0; cmds[i].cmdargs[j] != (char*)NULL; j++)
                    fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n", i, j,
                            cmds[i].cmdargs[j]);
                fprintf(stderr, "cmds[%d].cmdflag = %o\n", i, cmds[i].cmdflag);
            }
        }
#endif

        // Exit shell
        if (strcmp(cmds[0].cmdargs[0], "exit") == 0) {
            return 0;
        }

        // Jobs built-in command
        if (strcmp(cmds[0].cmdargs[0], "jobs") == 0) {
            list_jobs();
            continue;
        }

        // FG built-in command
        if (strcmp(cmds[0].cmdargs[0], "fg") == 0) {
            int jid;
            struct job* jb;

            if (cmds[0].cmdargs[1] != NULL) {
                if (cmds[0].cmdargs[1][0] == '%') {
                    jid = atoi(&cmds[0].cmdargs[1][1]);
                } else {
                    jid = atoi(cmds[0].cmdargs[1]);
                }
                jb = get_job_by_jid(jid);
            } else {
                for (int i = MAXJOBS - 1; i >= 0; i--) {
                    if (jobs[i].pid != 0) {
                        jb = &jobs[i];
                        break;
                    }
                }
            }

            if (jb == NULL || jb->pid == 0) {
                fprintf(stderr, "fg: no such job\n");
                continue;
            }

            printf("%s\n", jb->cmdline);

            if (jb->state == STOPPED) {
                kill(-jb->pgid, SIGCONT);
            }
            jb->state = RUNNING;

            if (isatty(STDIN_FILENO)) {
                tcsetpgrp(STDIN_FILENO, jb->pgid);
            }
            handler_child(jb->pid, jb->cmdline);
            if (isatty(STDIN_FILENO)) {
                tcsetpgrp(STDIN_FILENO, shell_pgid);
            }
            continue;
        }

        // BG built-in command
        if (strcmp(cmds[0].cmdargs[0], "bg") == 0) {
            int jid;
            struct job* jb;

            if (cmds[0].cmdargs[1] != NULL) {
                if (cmds[0].cmdargs[1][0] == '%') {
                    jid = atoi(&cmds[0].cmdargs[1][1]);
                } else {
                    jid = atoi(cmds[0].cmdargs[1]);
                }
                jb = get_job_by_jid(jid);
            } else {
                jb = NULL;
                for (int i = MAXJOBS - 1; i >= 0; i--) {
                    if (jobs[i].pid != 0 && jobs[i].state == STOPPED) {
                        jb = &jobs[i];
                        break;
                    }
                }
            }

            if (jb == NULL || jb->pid == 0) {
                fprintf(stderr, "bg: no such job\n");
                continue;
            }

            if (jb->state != STOPPED) {
                fprintf(stderr, "bg: job already running\n");
                continue;
            }

            printf("[%d]   %s &\n", jb->jid, jb->cmdline);

            kill(-jb->pgid, SIGCONT);
            jb->state = RUNNING;
            continue;
        }

        // 13-14
        for (i = 0; i < ncmds; i++) {
            child_pid = fork();
            switch (child_pid) {
                case -1:
                    perror("Error fork");
                    exit(1);
                case 0:
                    setpgid(0, 0);

                    if (bkgrnd) {
                        ignore_signals();
                    } else {
                        activate_signals();
                    }

                    if (infile && i == 0) {
                        file_operation(infile, 0);
                    }
                    if (outfile && i == ncmds - 1) {
                        file_operation(outfile, 1);
                    }
                    if (appfile && i == ncmds - 1) {
                        file_operation(appfile, 2);
                    }

                    execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
                    perror("Error execvp");
                    exit(1);
                default:
                    setpgid(child_pid, child_pid);

                    if (bkgrnd) {
                        int jid = add_job(child_pid, child_pid, RUNNING,
                                          original_line);
                        fprintf(stderr, "[%d] %d\n", jid, child_pid);
                    } else {
                        if (isatty(STDIN_FILENO)) {
                            tcsetpgrp(STDIN_FILENO, child_pid);
                        }
                        handler_child(child_pid, original_line);
                        if (isatty(STDIN_FILENO)) {
                            tcsetpgrp(STDIN_FILENO, shell_pgid);
                        }
                    }
                    break;
            }

            /*  FORK AND EXECUTE  */
        }

    } /* close while */

    return 0;
}

/* PLACE SIGNAL CODE HERE */