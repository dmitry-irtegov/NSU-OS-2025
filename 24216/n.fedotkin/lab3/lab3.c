#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void checkOpenFile(const char* nameFile) {
    FILE* file = fopen(nameFile, "rw");
    if (file == NULL) {
        perror("Error opening file");
    }
    printf("File opened successfully\n");
    fclose(file);
}

int main() {
    const char* nameFile = "file.txt";

    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    
    checkOpenFile(nameFile);

    int res = setuid(getuid());
    if (res == -1) {
        perror("Error setting UID");
        exit(1);
    }
    
    printf("UID changed successfully\n");

    printf("Real UID after setting: %d\n", getuid());
    printf("Effective UID after setting: %d\n", geteuid());

    checkOpenFile(nameFile);

    exit(0);
}
