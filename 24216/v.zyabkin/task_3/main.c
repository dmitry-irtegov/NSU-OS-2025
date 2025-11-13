#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>


void print_and_open(const char *filename) {
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen failed");
    } else {
        fclose(file);
    }
}


int main() {
    print_and_open("test.txt");

    uid_t real_uid = getuid();
    if (setuid(real_uid) == -1) {
        perror("setuid failed");
        exit(EXIT_FAILURE);
    }

    print_and_open("test.txt");

    return 0;
}

