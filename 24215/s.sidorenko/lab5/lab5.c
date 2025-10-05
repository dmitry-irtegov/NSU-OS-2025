#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct LineInf_T {
    off_t offset;
    int line;
} LineInf;

int main()
{
    int fd = open("file1.txt", O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    LineInf *lines = malloc(sizeof(LineInf) * 128);
    if (!lines) 
    {
        perror("malloc");
        return 1;
    }
    size_t capacity = 128;
    size_t count = 0;
    char c;
    off_t pos = 0;

    lines[count].line = count;
    lines[count].offset = 0;
    count++;

    while (read(fd, &c, 1) == 1) 
    {
        pos++;
        if (c == '\n') 
        {
            lines[count].line = count;
            lines[count].offset = pos;
            count++;
            if (count >= capacity) 
            {
                capacity *= 2;
                lines = realloc(lines, sizeof(LineInf) * capacity);

                if (!lines) 
                {
                    perror("realloc");
                    return 1;
                }
            }
        }
    }

    printf("\nEnter line number:\n");
    int ansline;
    scanf("%d", &ansline);
    while (ansline != 0)
    {
        if (ansline < 0 || ansline > count)
        {
            printf("Invalid line number!\n");
            printf("Enter line number:\n");
            scanf("%d", &ansline);
            continue;
        }
        lseek(fd, lines[ansline-1].offset, SEEK_SET);
        off_t anslength;
        if (ansline == count)
            anslength = pos - lines[ansline-1].offset;
        else
            anslength = lines[ansline].offset - lines[ansline-1].offset;

        char *buffer = malloc(anslength + 1);
        if (!buffer) 
        { 
            perror("malloc"); 
            return 1; 
        }
        ssize_t n = read(fd, buffer, anslength);
        buffer[n] = '\0';
        printf("%s", buffer);
        free(buffer);

        printf("\nEnter line number:\n");
        scanf("%d", &ansline);
    }
    printf("\nСompleted.\n");

    free(lines);
    close(fd);
}