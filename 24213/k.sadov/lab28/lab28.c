#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>

#define COUNT 100
#define PER_LINE 10

int main() {

    FILE *pipefd[2];
    int i, value;

    srand((unsigned int)time(NULL));

    if (p2open("sort -n", pipefd) == -1) {
        perror("p2open");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < COUNT; i++) {
        fprintf(pipefd[0], "%d\n", rand() % 100);
    }

    fclose(pipefd[0]);

    i = 0;
    while (fscanf(pipefd[1], "%d", &value) == 1) {
        printf("%2d", value);

        if ((i + 1) % PER_LINE == 0)
            putchar('\n');
        else
            putchar(' ');

        ++i;
    }

    if (pclose(pipefd[1]) == -1) {
        perror("pclose failed");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
