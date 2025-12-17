#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>

/*
28. Генератор случайных чисел
Напишите программу, которая генерирует сортированный список из ста случайных
чисел в диапазоне от 0 до 99. Распечатайте числа по десять в строке. Используйте
p2open(3), чтобы запустить sort(1) и rand(3) и srand(3) для генерации случайных чисел.
*/

int main() {
    FILE *fp[2];

    srand(time(NULL));

    if (p2open("sort -n", fp) == -1) {
        perror("p2open failed");
        return 1;
    }

    for (int i = 0; i < 100; i++) {
        int num = rand() % 100;
        fprintf(fp[0], "%d\n", num);
    }

    fclose(fp[0]);

    int num;
    int count = 0;

    while (fscanf(fp[1], "%d", &num) == 1) {
        printf("%02d ", num);
        if (count % 10 == 9) {
            printf("\n");
        }
        count++;
    }

    p2close(fp);
    
    return 0;
}