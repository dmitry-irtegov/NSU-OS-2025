#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAXLINE 1024

int main(int argc, char *argv[]) {
    FILE *fp_in;
    int fd[2];
    pid_t pid;
    char line[MAXLINE];
    
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        exit(1);
    }
    
    if ((fp_in = fopen(argv[1], "r")) == NULL) {
        perror(argv[1]);
        exit(2);
    }
    
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(3);
    }
    
    if ((pid = fork()) == -1) {
        perror("fork");
        exit(4);
    }
    
    if (pid == 0) { 
        close(fd[1]); 
        dup2(fd[0], 0);
        close(fd[0]);
        
        execlp("/usr/bin/wc", "wc", "-l", (char *)0);
        perror("execlp");
        exit(127);
    } else {  
        close(fd[0]); 
        
        while (fgets(line, MAXLINE, fp_in) != NULL) {
            if (strcmp(line, "\n") == 0) {
                write(fd[1], line, strlen(line));
            }
        }
        
        close(fd[1]);
        fclose(fp_in);
        wait(NULL);
    }
    
    return 0;
}
