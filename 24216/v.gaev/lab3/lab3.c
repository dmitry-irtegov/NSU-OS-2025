#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void processing_file()
{
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    *file = fopen("file", "r");
    if (file == NULL) {
        perror("Error opening file 2");
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
    get_id();
    
    processing_file();
    return 0;
}
