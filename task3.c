#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
int main() {
    FILE *file;

    printf("BEFORE SETUID\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    printf("\n");

    file = fopen("test3.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
    }
    else {
        printf("File opened!\n");
        fclose(file);
    }
    printf("\n");

    if (setuid(getuid()) == -1) {
        perror("Error in setuid");
        return 1;
    }

    printf("AFTER SETUID\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    printf("\n");

    file = fopen("test3.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
    }
    else {
        printf("File opened!");
        fclose(file);
    }

    return 0;
}
