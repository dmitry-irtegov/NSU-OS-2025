#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define COMMAND_SORT "sort -n"

int main() {
    srand(time(NULL));

    FILE* fp[2];
    if (p2open(COMMAND_SORT, fp) == -1) {
        perror("p2open error");
        exit(EXIT_FAILURE);
    }

    int counter = 100;
    while (counter-- > 0) {
        if (fprintf(fp[0], "%d\n", rand() % 100) < 0) {
            perror("fprintf error");
            p2close(fp);
            exit(EXIT_FAILURE);
        }
    }

    if (fclose(fp[0]) == EOF) {
        perror("fclose error");
        p2close(fp);
        exit(EXIT_FAILURE);
    }

    int num, count = 0;
    while (fscanf(fp[1], "%d", &num) != EOF) {
        count++;
        printf("%d%s", num, (count % 10 == 0) ? "\n" : " ");
    }
    printf("\n");

    if (p2close(fp) == -1) {
        perror("p2close error");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}