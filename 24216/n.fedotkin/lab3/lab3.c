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

    uid_t uid = getuid();
    uid_t euid = geteuid();

    printf("Real UID: %d\n", uid);
    printf("Effective UID: %d\n", euid);

    FILE* file = fopen(nameFile, "rw");
    checkOpenFile(file);

    int res = setuid(uid);
    if (res == -1) {
        perror("Error setting UID");
        exit(1);
    } else {
        printf("UID changed successfully\n");
    }

    printf("Real UID after setting: %d\n", uid);
    printf("Effective UID after setting: %d\n", euid);

    file = fopen(nameFile, "rw");
    checkOpenFile(file);
    
    exit(0);
}
