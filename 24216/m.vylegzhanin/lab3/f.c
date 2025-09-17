#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(void) {
    FILE *fp;
    uid_t ruid, euid;

    ruid = getuid(); // реальный UID
    euid = geteuid(); // эффективный UID
    printf("Real UID: %d, Effective UID: %d\n", ruid, euid);

    fp = fopen("file", "r+");
    if (fp == NULL) {
        perror("first fopen");
    } else {
        printf("File opened successfully.\n");
        fclose(fp);
    }

    if (setuid(ruid) == -1) {
        perror("setuid");
        exit(EXIT_FAILURE);
    }

    ruid = getuid(); // реальный UID
    euid = geteuid(); // эффективный UID
    printf("Real UID: %d, Effective UID: %d\n", ruid, euid);

    fp = fopen("file", "r+");
    if (fp == NULL) {
        perror("first fopen");
    } else {
        printf("File opened successfully.\n");
        fclose(fp);
    }

    return 0;
}
