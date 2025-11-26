#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void) {
    FILE *fp;
    int status;
    const char *message = "cheCKCK tesTTTTT\n";
    fp = popen("tr [:lower:] [:upper:]", "w");
    
    if (fp == NULL) {
        perror("popen failed");
        exit(EXIT_FAILURE);
    }

    if (fputs(message, fp) == EOF) {
        perror("fputs failed");
        pclose(fp);
        exit(EXIT_FAILURE);
    }
    
    status = pclose(fp);

    if (status == -1) {
        perror("pclose failed");
        exit(EXIT_FAILURE);
    }
    
    if (WIFEXITED(status)) {
        printf("\nchild process finished sucsess %d\n", WEXITSTATUS(status));
    } else {
        printf("\nerror finished child process\n");
    }

    return 0;
}