#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#define SIZE 100
#define BUFFSIZE 10

int main() {
    // устанавливаем seed для генерации случаных чисел 
    srand((unsigned int) time(NULL));

    int numbers[SIZE] = {0}; 
    for (size_t i = 0; i < SIZE; ++i)
        numbers[i] = rand() % 100; 
    FILE *pipe[2];
    if (p2open("sort -n -d", pipe) == -1) {
        printf("Fork/pipe throws some error\n"); 
    } else {
        char buff[BUFFSIZE] = {'\0'};
        for (size_t i = 0; i < SIZE; ++i) {
            // делаем из числа строку, которую будем подавать на вход в sort 
            if (snprintf(buff, BUFFSIZE,"%d\n", numbers[i]) < 0) {
                printf("[snprintf] error\n");
                if (p2close(pipe) == -1) 
                    printf("[p2close] error\n");
                return 0;   
            } 
            // подаём число на вход в sort 
            if (fputs(buff, pipe[0]) == EOF) {
                printf("[fputs] error\n");
                if (p2close(pipe) == -1) 
                    printf("[p2close] error\n");
                return 0;
            }
        }
        // закрываем вход для sort, обозначая конец файла
        if (fclose(pipe[0]) == EOF) {  
            printf("[fclose] error\n");
            if (p2close(pipe) == -1) 
                printf("[p2close] error\n");
            return 0;
        }
        // выводим числа в нужном формате
        for (int i = 1; fgets(buff, BUFFSIZE, pipe[1]) != NULL; ++i) {
            if (strchr(buff, (int) '\n') != NULL)
                *(strchr(buff, (int) '\n')) = '\0'; 
            printf("%s\t", buff); 
            if (i % 10 == 0)
                printf("\n");
        }
        // закрываем канал 
        if (p2close(pipe) == -1) {
            printf("[pclose] error\n");
            return 0;  
        } 
    }
}
