#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "shell.h"
#include <stdlib.h>
char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char *argv[])
{
    register int i;
    char line[1024]; /*  allow large command lines  */
    int ncmds;
    char prompt[50]; /* shell prompt */

    pid_t child_pid = -1;

    /* PLACE SIGNAL CODE HERE */

    sprintf(prompt, "[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0)
    { /*
until eof  */
        if ((ncmds = parseline(line)) <= 0)
            continue; /* read next line */
#ifdef DEBUG
        {
            int i, j;
            for (i = 0; i < ncmds; i++)
            {
                for (j = 0; cmds[i].cmdargs[j] != (char *)NULL; j++)
                    fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n",
                            i, j, cmds[i].cmdargs[j]);
                fprintf(stderr, "cmds[%d].cmdflag = %o\n", i,
                        cmds[i].cmdflag);
            }
        }
#endif

        for (i = 0; i < ncmds; i++)
        {
            child_pid = fork();
            switch (child_pid)
            {
            case -1:
                perror("Error fork");
                exit(1);
            case 0:
                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
                perror("Error execvp");
                exit(1);
            default:
                if (bkgrnd)
                    printf("Background process PID: %d\n", child_pid);
                else
                    handler_child(child_pid);
                break;
            }

            /*  FORK AND EXECUTE  */
        }

    } /* close while */

    return 0;
}

/* PLACE SIGNAL CODE HERE */