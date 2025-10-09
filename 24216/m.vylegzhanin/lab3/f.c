#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void OpenFile(FILE *fp){
    fp = fopen("file", "r+");
    if (fp == NULL) {
        perror("fopen crashed");
    } else {
        printf("File opened successfully.\n");
        fclose(fp);
    }
}

int main(void) {
    FILE *fp;
    uid_t ruid, euid;

    ruid = getuid(); // реальный UID
    euid = geteuid(); // эффективный UID
    printf("Real UID: %d, Effective UID: %d\n", ruid, euid);

    OpenFile(fp);

    if (setuid(ruid) == -1) {
        perror("setuid is crashed");
        exit(EXIT_FAILURE);
    }

    ruid = getuid(); // реальный UID
    euid = geteuid(); // эффективный UID
    printf("Real UID: %d, Effective UID: %d\n", ruid, euid);

    OpenFile(fp);

    return 0;
}
