#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void open_file(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }
    printf("File opened successfully\n");
    fclose(file);
}

int main() {
    uid_t real = getuid();
    uid_t effective = geteuid();
    char *filename = "file";

    printf("Real UID: %d\n", real);
    printf("Effective UID: %d\n", effective);
    open_file(filename);

    if (setuid(real) != 0) {
        perror("Error setting real UID");
        return 1;
    }

    effective = geteuid();
    printf("Real UID: %d\n", real);
    printf("Effective UID: %d\n", effective);
    open_file(filename);

    return 0;
}