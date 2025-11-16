#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include "shell.h"

char *infile, *outfile, *appfile;
struct command cmds[MAXCMDS];
char bkgrnd;

int main(int argc, char *argv[]) {
	register int i;
	char line[1024];		/* allow large command lines */
	int ncmds;
	char prompt[50];		/* shell prompt */

	/* PLACE SIGNAL CODE HERE  */

	sprintf(prompt, "[%s] ", argv[0]);
	while (promptline(prompt, line, sizeof(line)) > 0) {		/* until eof */
		if ((ncmds = parseline(line)) <= 0) {
			continue;		/* read next line */
		}
	
	#ifdef DEBUG 
	{
		int i, j;
		for (int i = 0; i < ncmds; i++) {
			for (int j = 0; cmds[i].cmdargs[j] != (char*)NULL; j++) {
			fprintf(stderr, "cmds[%d].cmdargs[%d] = %s\n", i, j, cmds[i].cmdargs[j]);
			}\
			fprintf(stderr, "cmds[%d].cmdflag = %o\n", i, cmds[i].cmdflag);
		}
	}
	#endif 
	for (i = 0; i < ncmds; i++) {
			if (!cmds[i].cmdargs[0]) {
				fprintf(stderr, "Empty command, skipping.\n");
  			continue;
  		}
			pid_t pid = fork();
			int status;
			switch(pid) {
				case -1:
					perror("Fork failed.");
					exit(1);
				case 0:
					if (infile && i == 0) {
						int fd = open(infile, O_RDONLY);
						if (fd < 0) {
							perror("Failed to open file\n");
							exit(1);
						}
						if (dup2(fd, STDIN_FILENO) < 0) {
							perror("Dup2 failed.");
							exit(1);
						}
						close(fd);
					}
					if ((outfile || appfile) && i == ncmds - 1) {
						int fd;
						if (outfile) {
							fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
						} else {
							fd = open(appfile, O_WRONLY | O_CREAT | O_APPEND, 0666);
						}
						if (fd < 0) {
							perror("Failed to open file\n");
							exit(1);
						}
						if (dup2(fd, STDOUT_FILENO) < 0) {
							perror("Dup2 failed.");
							exit(1);
						}
						close(fd);
					}
					execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
					perror("Execvp failed.");
					exit(1);
				default:
					if (bkgrnd) {
    				printf("[Background pid %d]\n", pid);
					} else {
						if (waitpid(pid, &status, 0) == -1) {
							perror("Waitpid failed.");
							exit(1);
						}
					}
				}
			}
		}		/* close while */
	return 0;
}

