#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <time.h>

int main() {

    srand(time(NULL));

    FILE* pipes[2];
    if (p2open("sort -n", pipes) == -1) {
        perror("p2open");
        return 1;
    }

    for (int i = 0; i < 100; i++) {
        fprintf(pipes[0], "%d\n", rand() % 100);
    }
    fclose(pipes[0]);

    int num;
    int printed = 0;
    while (fscanf(pipes[1], "%d", &num) != EOF) {
        printf("%2d", num);
        
        if (printed % 10 == 9) {
            putchar('\n');
        } else {
            putchar(' ');
        }
        
        printed++;
    }

    return 0;
}
