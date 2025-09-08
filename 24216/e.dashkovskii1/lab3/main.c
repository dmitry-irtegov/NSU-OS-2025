#include <stdio.h>
#include <unistd.h>

int main() {
    const char *filename = "file.txt";
    FILE *file;

    printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());

    file = fopen(filename, "r+");
    if (!file) {
        perror("fopen");
    } else {
        fclose(file);
    }

    if (setuid(getuid()) != 0) {
        perror("setuid");
        return 1;
    }

    printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());

    file = fopen(filename, "r+");
    if (!file) {
        perror("fopen");
    } else {
        fclose(file);
    }

    return 0;
}