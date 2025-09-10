#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

void openFile(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
    }
    else
    {
        printf("File opened successfully\n");
        fclose(file);
    }
}

int main()
{
    uid_t real = getuid();
    uid_t effective = geteuid();
    char *filename = "file";

    printf("Real UID: %d\n", real);
    printf("Effective UID: %d\n", effective);
    openFile(filename);

    if (setuid(real) != 0)
    {
        perror("Error setting real UID");
        return 1;
    }

    printf("Real UID: %d\n", real);
    printf("Effective UID: %d\n", effective);
    openFile(filename);

    return 0;
}