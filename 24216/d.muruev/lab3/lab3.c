#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <sys/types.h>

void print_ids(const char* filename) {
    FILE* fp;

    uid_t ruid = getuid();
    uid_t euid = geteuid();
    
    printf("RUID: %d\n", ruid);
    printf("EUID: %d\n", euid);

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen error");
    } else {
        printf("File opened successfully\n");
        fclose(fp);
    }
}

int main() {
    char* filename = "data.txt";
    print_ids(filename);

    uid_t real_uid = getuid();
    if (setuid(real_uid) == -1) {
        perror("setuid error");
        exit(EXIT_FAILURE);
    }

    print_ids(filename);
    exit(0);
}