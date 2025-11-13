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
    char cmd_line[MAXLINE] = "";

    /* PLACE SIGNAL CODE HERE */
    ignore_signals();

    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("setpgid");
        exit(1);
    }

    set_terminal_foreground(shell_pgid);

    init_jobs();

    sprintf(prompt, "[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) { /* until eof  */

        cleanup_zombies();

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

        if (is_builtin(&cmds[0])) {
            execute_builtin(&cmds[0]);
            continue;
        }

        // 13-14
        pid_t pgid = 0;

        create_pipes(pipes, ncmds);
        for (i = 0; i < ncmds; i++) {
            child_pid = fork();
            switch (child_pid) {
                case -1:
                    perror("Error fork");
                    exit(1);
                case 0:
                    if (i == 0) {
                        setpgid(0, 0);
                    } else {
                        setpgid(0, pgid);
                    }

                    // if (bkgrnd) {
                    //     ignore_signals();
                    // } else {
                    //     activate_signals();
                    // }
                    activate_signals();

                    setup_pipe_input(pipes, i);
                    setup_pipe_output(pipes, i, ncmds);

                    close_all_pipes(pipes, ncmds);

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
                    if (i == 0) {
                        setpgid(child_pid, child_pid);
                        pgid = child_pid;
                    } else {
                        setpgid(child_pid, pgid);
                    }

                    break;
            }

            /*  FORK AND EXECUTE  */
        }
        close_all_pipes(pipes, ncmds);

        for (int k = 0; k < ncmds; k++) {
            for (int m = 0; cmds[k].cmdargs[m] != NULL; m++) {
                strcat(cmd_line, cmds[k].cmdargs[m]);
                strcat(cmd_line, " ");
            }
        }

        if (bkgrnd) {
            int jid = add_job(pgid, pgid, RUNNING, cmd_line);
            fprintf(stderr, "[%d] %d\n", jid, pgid);
        } else {
            set_terminal_foreground(pgid);

            handler_child(pgid, ncmds, cmd_line);

            set_terminal_foreground(shell_pgid);
        }
    } /* close while */

    return 0;
}

/* PLACE SIGNAL CODE HERE */