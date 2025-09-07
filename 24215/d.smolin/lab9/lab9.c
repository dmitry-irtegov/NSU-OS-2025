#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

void main() {
    pid_t pid = fork();
    if (pid < 0) {
	perror("fork");
	exit(1);
    }

    if (pid == 0) {
        execlp("cat", "cat", "file.txt", (char*)0);
        perror("execlp");
        _exit(127);
    }

    printf("Parent: first line before wait\n");

    int status;
//    if (waitpid(pid, &status, 0) < 0) {
//	perror("waitpid");
//	exit(1);
//  }

    printf("Parent: last line after child finished\n");
}
