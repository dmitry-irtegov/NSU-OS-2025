#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void checkOpenFile(FILE* file) {
    if (file == NULL) {
        perror("Error opening file");
    } else {
        printf("File opened successfully\n");
        (void)fclose(file);
    }
}

int main() {
    const char* nameFile = "file.txt";

    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    FILE* file = fopen(nameFile, "rw");
    checkOpenFile(file);

    int res = setuid(getuid());
    if (res == -1) {
        perror("Error setting UID");
        exit(1);
    } else {
        printf("UID changed successfully\n");
    }

    printf("Real UID after setting: %d\n", getuid());
    printf("Effective UID after setting: %d\n", geteuid());

    file = fopen(nameFile, "rw");
    checkOpenFile(file);
    
    exit(0);
}
