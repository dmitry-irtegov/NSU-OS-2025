#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int read_secret_file(char* filename)
{
    FILE* input = fopen(filename, "r");
    
    if(!input)
    {
        perror("File error");
        return -1;
    }

    printf("Reading file...\n");
    fclose(input);
    return 0;
}

int main()
{
    printf("Your UID is: %d\n", getuid());
    printf("Your EUID is: %d\n", geteuid());
    int flag_error = 0;
    
    uid_t original_euid = geteuid();
    
    if(geteuid() != getuid())
    {
        if(seteuid(getuid()) == -1)
        {
            perror("Fail to lower privileges");
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
    
    if(original_euid != getuid())
    {
        if(seteuid(original_euid) == -1)
        {
            perror("Fail to restore privileges!");
            flag_error = -1;
        }
        
        else
        {
            printf("Privileges restored:\n");
            printf("Now your UID is: %d\n", getuid());
            printf("Now your EUID is: %d\n", geteuid());
        }
    }
    
    return flag_error;
}