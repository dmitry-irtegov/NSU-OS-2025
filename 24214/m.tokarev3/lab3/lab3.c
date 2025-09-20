#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define FILE_PATH "./file"


void openFile(FILE *file)
{
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
}

void printUID()
{
    printf("Ruid: %u\nEuid: %u\n", getuid(), geteuid());
}



int main()
{
    FILE *file;

    printUID();

    openFile(file);

    if (setuid(getuid()) == -1)
    {
        perror("Setuid error\n");
        return 1;
    }

    printUID();

    openFile(file);

    return 0;
}