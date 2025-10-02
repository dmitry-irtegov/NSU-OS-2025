#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void fileOpen(const char *filename) {
    FILE *f = fopen(filename, "r+");
    if (f == NULL) {
        perror("can't open file");
        exit(1);
    } 
    printf("file opened successfully\n");
    fclose(f);
}

void printUID() {
    printf("Real UID = %d, Effective UID = %d\n", getuid(), geteuid());
}

int main(void) {
    const char *filename = "data.txt";

    printUID();
    fileOpen(filename);

    if (setuid(getuid()) != 0) {
        perror("setuid(getuid()) failed");
        exit(EXIT_FAILURE);
    }

    printUID();
    fileOpen(filename);

    return 0;
}