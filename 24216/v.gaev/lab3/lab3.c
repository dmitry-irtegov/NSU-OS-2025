#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>


void try_close(FILE *file)
{
    if (fclose(file) != 0) {
         perror("Error closing file");
    } 
    else {
         printf("File closed successfully.\n");
    }
}

void get_id()
{
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
}

int main() {
    get_id();
    FILE *file1 = fopen("file", "r");
    if (file1 == NULL) {
        perror("Error opening file 1");
    }
    try_close(file1);
    
    if (setuid(getuid()) == -1) {
        perror("setuid failed");
        exit(1);
    }

    printf("After setuid:\n");
    get_id();
    
    FILE *file2 = fopen("file", "r");
    if (file2 == NULL) {
        perror("Error opening file 2");
    }
    try_close(file2);
    
    return 0;
}
