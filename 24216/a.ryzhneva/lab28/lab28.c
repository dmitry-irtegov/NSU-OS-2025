#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>

#define NUM_COUNT 100
#define RANGE 100
#define LINE_BREAK 10

int main() {
    FILE *fp[2];
    int number;

    srand((unsigned int)time(NULL));

    if (p2open("sort -n", fp) == -1) {
        perror("p2open failed");
        return 1;
    }

    for (int i = 0; i < NUM_COUNT; i++) {
        number = rand() % RANGE;
        fprintf(fp[0], "%d\n", number);
    }

    fclose(fp[0]);

    int k = 0;
    while (fscanf(fp[1], "%d", &number) == 1) {
        printf("%02d ", number);
        
        k++;
        if (k % LINE_BREAK == 0) {
            printf("\n");
        }
    }

    p2close(fp);
    return 0;
}