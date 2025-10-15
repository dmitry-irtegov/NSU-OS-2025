#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void open_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }
    fclose(file);
}

int main() { 
    printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
    open_file("file");
    if (setuid(getuid()) == -1) {
        perror("Error setting UID");
        exit(1);
    }
    printf("Real UID: %d, Effective UID: %d (after setuid)\n", getuid(), geteuid());
    open_file("file");
    return 0;
}
