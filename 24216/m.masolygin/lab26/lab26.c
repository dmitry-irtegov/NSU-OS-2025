#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define BUF_SIZE 1024

int main() {
    char str[BUF_SIZE] = "bhsbhFHSASVNHFSDVvnbvdngsavsnFVSDV\n";
    int status;

    FILE* pipe_fp = popen("tr a-z A-Z", "w");
    if (pipe_fp == NULL) {
        perror("popen");
        return 1;
    }

    if (fputs(str, pipe_fp) == EOF) {
        perror("fputs");
        if (pclose(pipe_fp) == -1) {
            perror("pclose");
            exit(1);
        }
        return 1;
    }

    status = pclose(pipe_fp);
    if (status == -1) {
        perror("pclose");
        exit(1);
    }

    return 0;
}