#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void fileOpen(const char *filename) {
    printf("Real UID = %d, Effective UID = %d\n", getuid(), geteuid());
    FILE *f = fopen(filename, "r+");
    if (f == NULL) {
        perror("can't open file");
        exit(1);
    } 
    printf("file opened successfully\n");
    fclose(f);
}

int main(void) {
    const char *filename = "data.txt";

    fileOpen(filename);

    if (setuid(getuid()) != 0) {
        perror("setuid(getuid()) failed");
        exit(EXIT_FAILURE);
    }

    fileOpen(filename);

    return 0;
}