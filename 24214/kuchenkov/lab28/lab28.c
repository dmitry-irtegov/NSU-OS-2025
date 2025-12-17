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

    srand(time(NULL));

    if (p2open("sort -n", "w") == NULL) {
        perror("p2open failed");
        return 1;
    }

    for (int i = 0; i < 100; i++) {
        int num = rand() % 100;
        printf("%d\n", num);
    }

    p2close();
    
    return 0;
}