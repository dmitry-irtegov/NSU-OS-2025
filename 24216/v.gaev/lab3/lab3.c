#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void processing_file()
{
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    
    FILE *file = fopen("file", "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    if (fclose(file) != 0) {
        perror("Error closing file");
    } 
    
    printf("File closed successfully.\n");
}

int main() {
    processing_file();
    
    if (setuid(getuid()) == -1) {
        perror("setuid failed");
        exit(1);
    }

    printf("After setuid:\n");
    processing_file();
    
    return 0;
}
