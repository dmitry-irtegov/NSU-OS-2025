#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include "shell.h"
#include <stdlib.h>
#include <fcntl.h>

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char *argv[])
{
    register int i;
    char line[1024]; /*  allow large command lines  */
    int ncmds;
    char prompt[50];  /* shell prompt */

    sprintf(prompt, "[%s] ", argv[0]);

    while (promptline(prompt, line, sizeof(line)) > 0) {  /*until eof  */
        if ((ncmds = parseline(line)) <= 0)
            continue; /* read next line */
    

#ifdef DEBUG
    {
        for (int i = 0; i < ncmds; i++) 
        {
            for (int j = 0; cmds[i].cmdargs[j] != (char *) NULL; j++)
                {
                    fprintf(stderr, "cmd[%d].cmdargs[%d] = %s\n", 
                    i, j, cmds[i].cmdargs[j]);
                }
            fprintf(stderr, "cmds[%d].cmdflag = %o\n", i, cmds[i].cmdflag);
        }
    }
#endif
    {
        for (i = 0; i < ncmds; i++) 
        {
            pid_t pid = fork();

            switch (pid) {
            case -1:
                perror("fork");
                exit(1);
            case 0:
                if (infile != NULL && i == 0) {
                    int fd = open(infile, O_RDONLY);
                    if (fd < 0) {
                        perror("infile");
                        exit(1);
                    }

                    if (dup2(fd, STDIN_FILENO) < 0) {
                        perror("dup2 infile");
                        exit(1);
                    }
                    close(fd);
                }

                if ((outfile != NULL || appfile != NULL) && i == ncmds-1) {
                    int fd;
                    if (appfile != NULL) {
                        fd = open(appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    } 
                    else {
                        fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    }

                    if (fd < 0) {
                        perror("outfile/appfile");
                        exit(1);
                    }

                    if (dup2(fd, STDIN_FILENO) < 0) {
                        perror("dup2 infile");
                        exit(1);
                    }
                    close(fd);
                }
                

                if (cmds[i].cmdargs[0] == NULL) {
                    exit(0);
                }

                execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
                perror("cmds[i].cmdargs");
                exit(1);
            default:
                if (bkgrnd) {
                    printf("[bg] pid=%d\n", pid);
                }
                else {
                    int status;
                    if (waitpid(pid, &status, 0) < 0) {
                        perror("waitpid");
                    }   
                }
                break;
            }
        }   
    }
}
    return 0;
}

/* PLACE SIGNAL CODE HERE */

