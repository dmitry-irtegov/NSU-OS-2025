#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

void main()
{
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    FILE *f = fopen("file.txt", "r+");
    if (f == NULL) {
        perror("fopen");
    } else {
        char buf[128];
	if (fgets(buf, sizeof(buf), f)) {
            printf("%s\n", buf);
        }
        fclose(f);
    }

    if (seteuid(getuid()) < 0) {
        perror("setuid");
        exit(EXIT_FAILURE);
    }

    printf("\nafter setuid:\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    f = fopen("file.txt", "r+");
    if (f == NULL) {
        perror("fopen");
    } else {
        char buf[128];
	if (fgets(buf, sizeof(buf), f)) {
            printf("%s\n", buf);
        }
        fclose(f);
    }
}
