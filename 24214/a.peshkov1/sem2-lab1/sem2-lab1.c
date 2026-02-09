#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* thread_routine(void* arg) {
    for (int i = 1; i <= 10; i++) {
        printf("нить-ребёнок: строка %d\n", i);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_t thread;
    int result;
    result = pthread_create(&thread, NULL, thread_routine, NULL);

    if (result != 0) {
        fprintf(stderr, "Ошибка создания нити: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }
    for (int i = 1; i <= 10; i++) {
        printf("нить-родитель: строка %d\n", i);
    }
    pthread_exit(NULL);
}