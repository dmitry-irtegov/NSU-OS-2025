#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>

int main() {
    FILE *fp[2]; 

    if (p2open("sort -n", fp) == -1) {
        perror("Error p2open");
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    for (int i = 0; i < 100; i++) {

        fprintf(fp[0], "%d\n", rand() % 100);
    }

    fclose(fp[0]);

    int number;
    int count = 0;

    printf("Sorted list of random numbers:\n");


    while (fscanf(fp[1], "%d", &number) != EOF) {

        printf("%3d ", number);
        count++;

        if (count % 10 == 0) {
            printf("\n");
        }
    }

    if (count % 10 != 0) {
        printf("\n");
    }

    fclose(fp[1]);

    return EXIT_SUCCESS;
}