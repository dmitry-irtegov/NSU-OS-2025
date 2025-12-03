#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MSGSIZE 256

int main(int argc, char *argv[]) {
    int fd[2];
    pid_t pid;
    char msgout[MSGSIZE] = "Hello, World! I love Osi!";
    char msgin[MSGSIZE];
    int n;
    
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }
    
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(2);
    }
    
    if (pid > 0) {
        close(fd[0]);
        if (write(fd[1], msgout, strlen(msgout) + 1) == -1) {
            perror("write");
            exit(3);
        }
        
        close(fd[1]);
        wait(NULL);
        
    } else {
        close(fd[1]); 
        if ((n = read(fd[0], msgin, MSGSIZE)) == -1) {
            perror("read");
            exit(4);
        }
        
        close(fd[0]);
        for (int i = 0; i < n && msgin[i] != '\0'; i++) {
            msgin[i] = toupper((unsigned char)msgin[i]);
        }
        printf("Обработанное сообщение:\n%s\n", msgin);
        exit(0);
    }
    
    return 0;
}
//test