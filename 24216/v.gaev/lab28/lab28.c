#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>

int main(void) {
    FILE *fp[2];   
    int i, number;
    int count = 0;
    srand((unsigned) time(NULL));
    if (p2open("sort -n", fp) == -1) {
        perror("p2open failed");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 100; i++) {
        int r = rand() % 100; 
        fprintf(fp[0], "%d\n", r);
    }
    fclose(fp[0]);
    while (fscanf(fp[1], "%d", &number) == 1) {
        printf("%3d ", number); 
        count++;
        if (count % 10 == 0) {
            printf("\n");
        }
    }
    if (count % 10 != 0) {
        printf("\n");
    }
    p2close(fp);
    return 0;
}
