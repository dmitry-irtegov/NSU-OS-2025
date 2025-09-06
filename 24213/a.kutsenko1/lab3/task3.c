#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    FILE *file;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    char *filename = argv[1];

    printf("BEFORE SETUID\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    printf("\n");

    file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
    }
    else {
        printf("File opened successfully!\n");
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

    file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
    }
    else {
        printf("File opened successfully!\n");
        fclose(file);
    }
    
    return 0;
}
