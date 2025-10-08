#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>


void print_user_ids() {
    uid_t real_uid = getuid();
    uid_t effective_uid = geteuid();

    printf("Real UID: %d.\n", real_uid);
    printf("Effective UID: %d.\n", effective_uid);
}

int main() {
    print_user_ids();

    FILE *file = fopen("test.txt", "r");
    if (file) {
        fclose(file);
    } else {
        perror("fopen failed");
    }

    uid_t real_uid = getuid();
    if (setuid(real_uid) == -1) {
        perror("setuid failed");
        exit(EXIT_FAILURE);
    }

    print_user_ids();

    exit(0);
}
