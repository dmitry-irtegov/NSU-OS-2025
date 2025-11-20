#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int open_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return -1;
    }
    fclose(file);
    return 0;
}

int main() { 
    printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
    if (open_file("file") == -1) {
        perror("Error opening file (before setuid)");
        return 1;
    }
    if (setuid(getuid()) == -1) {
        perror("Error setting UID");
        exit(1);
    }
    printf("Real UID: %d, Effective UID: %d (after setuid)\n", getuid(), geteuid());
    if (open_file("file") == -1) {
        perror("Error opening file (after setuid)");
        return 1;
    }
    return 0;
}
