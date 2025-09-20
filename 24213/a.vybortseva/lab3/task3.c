#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

int main()
{
    FILE *file;

    printf("real id: %u\n", getuid());
    printf("effective id: %u\n", geteuid());
    
    if ((file = fopen("file.txt", "a+")) == NULL)
    {
        perror("Cannot open file.txt");
        return 1;
    } else {
        fprintf(file, "1. vse ok\n");
        fclose(file);
    }

    if (seteuid(getuid()) == -1)
    {
        perror("seteuid failed");
        return 1;
    }

    printf("new real id: %u\n", getuid());
    printf("new effective id: %u\n", geteuid());
    
    if ((file = fopen("file.txt", "a+")) == NULL)
    {
        perror("Cannot open file.txt");
        return 1;
    } else {
        fprintf(file, "2. vse ok\n");
        fclose(file);
    }
   
    return 0;
}
