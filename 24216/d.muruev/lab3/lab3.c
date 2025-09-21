#include <stdio.h>
#include <unistd.h>   
#include <sys/types.h>

void print_ids(const char* filename) {
    FILE* fp;

    print("RUID: %d\n", getuid());
    print("EUID: %d\n", geteuid());

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen error");
        return;
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
        exit(EROR_);
    }

    print_ids(filename);
    exit(0);
}