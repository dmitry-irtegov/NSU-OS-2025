#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void fileOpen(const char *filename) {
    FILE *f = fopen(filename, "r+");
    if (f == NULL) {
        perror("Can't open file");
    } else {
        printf("Success open file.\n");
        fclose(f);
    }
}

void printUID() {
    printf("Real UID = %d, Effective UID = %d\n", getuid(), geteuid());
}

int main(void) {
    const char *filename = "data.txt";

    printUID();
    open(filename);

    if (setuid(getuid()) == -1) {
        perror("Ошибка setuid");
        exit(EXIT_FAILURE);
    }

    printUID();
    open(filename);

    return 0;
}