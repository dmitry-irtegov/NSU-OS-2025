#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>

int main() {
    FILE *filePointer[2];

    if (p2open("sort -n", filePointer) == -1) {
        perror("error p2open");
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    for (int i = 0; i < 100; i++) {
        fprintf(filePointer[0], "%d\n", rand() % 100);
    }

    if (fclose(filePointer[0]) == EOF){
        perror("failed close filePointer[0]");
        exit(EXIT_FAILURE);
    }

    int number,count = 0;
    while (fscanf(filePointer[1], "%d", &number) != EOF) {
        printf("%d ", number);
        count++;
        if (count % 10 == 0) {
            printf("\n");
        }
    }

    if (fclose(filePointer[1]) == EOF){
        perror("failed close filePointer[1]");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}