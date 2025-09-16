#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

void open_file(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
    }
    else
    {
        printf("File opened successfully\n");
        if (fclose(file) != 0)
        {
            perror("Error closing file");
        }
    }
}

int main()
{
    uid_t real = getuid();
    uid_t effective = geteuid();
    char *filename = "file";

    printf("Real UID: %d\n", real);
    printf("Effective UID: %d\n", effective);
    open_file(filename);

    if (setuid(real) != 0)
    {
        perror("Error setting real UID");
        return 1;
    }

    effective = geteuid();
    printf("Real UID: %d\n", real);
    printf("Effective UID: %d\n", effective);
    open_file(filename);

    return 0;
}