#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>

int main(void) {
    FILE *pipes[2];
    int val;
    int printed = 0;

    srand((unsigned)time(NULL));

    if (p2open("sort -n", pipes) < 0) {
        perror("p2open");
        return 1;
    }

    for (int k = 0; k < 100; k++) {
        fprintf(pipes[0], "%d\n", rand() % 100);
    }
    fclose(pipes[0]);

    while (fscanf(pipes[1], "%d", &val) == 1) {
        printf("%3d ", val);
        printed++;
        if (printed % 10 == 0)
            putchar('\n');
    }
    if (printed % 10 != 0)
        putchar('\n');

    p2close(pipes);
    return 0;
}
