#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define FILE_PATH "./file"


int main()
{
    uid_t ruid = getuid();
    uid_t euid = geteuid();

    printf("Ruid: %u\nEuid: %u\n", ruid, euid);

    FILE *file = fopen(FILE_PATH, "r");
    if (!file)
    {
        perror("Open error\n");
    }
    else
    {
        printf("Open success\n");
        fclose(file);
    }

    if (setuid(ruid) == -1)
    {
        perror("Setuid error\n");
        return 1;
    }

    ruid = getuid();
    euid = geteuid();

    printf("Ruid: %u\nEuid: %u\n", ruid, euid);

    file = fopen(FILE_PATH, "r");
    if (!file)
    {
        perror("Open error\n");
    }
    else
    {
        printf("Open success\n");
        fclose(file);
    }

    return 0;
}