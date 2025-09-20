#include <stdio.h>
#include <unistd.h>

void try_open(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen failed");
    } else {
        fclose(file);
    }
}

int main() {
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    try_open("file.txt");

    if (setuid(getuid()) != 0) {
        perror("setuid failed");
        return 1;
    }

    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    try_open("file.txt");

    return 0;
}