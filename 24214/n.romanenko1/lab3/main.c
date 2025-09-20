#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

int read_secret_file(char* fiilename)
{
    FILE* input = fopen(fiilename, "r");
    if(!input)
    {
        perror("File error");
        return -1;
    }

    else
    {
        printf("Reading file...\n");
        fclose(input);
    }

    return 0;
}

int main()
{
    printf("Your UID is: %d\n", getuid());
    printf("Your EUID is: %d\n", geteuid());
    int flag_error = 0;

    if(geteuid() != 1000)
    {
        if(setuid(0) == -1)
        {
            perror("Fail to evaluate root privilages");
            flag_error = -1;
        }

        else if(seteuid(1000) == -1)
        {
            perror("Fail to evaluate privilages");
            flag_error = -1;
        }

        else
        {
            printf("Now your UID is: %d\n", getuid());
            printf("Now your EUID is: %d\n", geteuid());
        }
    }

    if(read_secret_file("Private.txt") == -1)
    {
        flag_error = -1;
    }

    if(getuid() != geteuid())
    {
        printf("Your privilages was evaluated!\n");
        if(seteuid(getuid()) == -1)
        {
            perror("Fail to restore privilages!");
            flag_error = -1;
        }

        printf("Now your UID is: %d\n", getuid());
        printf("Now your EUID is: %d\n", geteuid());
    }

    return flag_error;
}