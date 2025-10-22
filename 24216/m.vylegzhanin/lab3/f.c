#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void OpenFile(void) {
    uid_t ruid = getuid();   // реальный UID
    uid_t euid = geteuid();  // эффективный UID
    printf("Real UID: %d, Effective UID: %d\n", ruid, euid);

    FILE *fp = fopen("file", "r+");
    if (fp == NULL) {
        perror("fopen failed");
    } else {
        printf("File opened successfully.\n");
        fclose(fp);
    }
}

int main(void) {
    uid_t ruid = getuid();
    uid_t euid = geteuid();

    printf("Before setuid:\n");
    OpenFile();

    if (setuid(ruid) == -1) {
        perror("setuid failed");
        exit(EXIT_FAILURE);
    }

    printf("After setuid:\n");
    OpenFile();

    return 0;
}
