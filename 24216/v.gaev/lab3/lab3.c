#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void try_close(int file)
{
    if (file == NULL) {
        perror("Error opening file");
    }
    else
    {
        fclose(file);
        printf("File opened and closed successfully.\n");
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
    try_close(file1);
    if (setuid(getuid()) == -1) {
        perror("setuid failed");
        exit(1);
    }

    printf("After setuid:\n");
    get_id();
    FILE *file2 = fopen("file", "r");
    try_close(file2);
}
