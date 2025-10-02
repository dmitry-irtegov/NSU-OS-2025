#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void try_open(const char *filename) {
    printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
    FILE *file = fopen(filename, "r+");
    if (!file) {
        perror("fopen");
        return;
    }
    fclose(file);
}

int main() {
    const char *filename = "file.txt";

    try_open(filename);

    if (setuid(getuid()) != 0) {
        perror("setuid");
        exit(1);
    }

    try_open(filename);

    exit(0);
}
