#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h> 

int main() {
    FILE *fp[2]; 
    int i, num;

    srand(time(NULL));

   
    if (p2open("sort -n", fp) == -1) {
        perror("Ошибка p2open");
        exit(1);
    }

    for (i = 0; i < 100; i++) {
        int r = rand() % 100; 
        fprintf(fp[0], "%d\n", r);
    }

    fclose(fp[0]);


    int count = 0;
    while (fscanf(fp[1], "%d", &num) == 1) {
        printf("%3d ", num); 
        count++;

        if (count % 10 == 0) {
            printf("\n");
        }
    }

    p2close(fp);

    return 0;
}