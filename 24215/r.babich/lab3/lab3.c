#include <stdio.h>
#include <unistd.h>

#define FILE_PATH "./file"

void print_ids() {
    printf("ruid: %u\n", getuid());
    printf("euid: %u\n", geteuid());
}

void check_open_file() {
    FILE *file = fopen(FILE_PATH, "r");

    if (!file) {
        perror("File could not be opened");
        return;
    } 
    printf("File opened successfully\n\n");
    fclose(file);
    
}

int main() {
    print_ids();
    check_open_file();

    if (setuid(getuid()) == -1) {
        perror("Failed to set uid");
    }

    print_ids();
    check_open_file();

    return 0;
}
