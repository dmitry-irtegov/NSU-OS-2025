#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    FILE* fp[2];

    if (p2open("sort -n", fp) == -1) {
        perror("p2open");
        return 1;
    }

    srand(time(NULL));

    for (int i = 0; i < 100; i++) {
        int num = rand() % 100;
        if (fprintf(fp[0], "%d\n", num) < 0) {
            perror("fprintf");
            p2close(fp);
            return 1;
        }
    }

    if (fclose(fp[0]) == EOF) {
        perror("fclose");
        p2close(fp);
        return 1;
    }

    int number, count = 0;
    while (fscanf(fp[1], "%d", &number) != EOF) {
        printf("%d ", number);
        count++;
        if (count % 10 == 0) {
            printf("\n");
        }
    }
    printf("\n");

    if (p2close(fp) == -1) {
        perror("p2close");
        return 1;
    }

    return 0;
}