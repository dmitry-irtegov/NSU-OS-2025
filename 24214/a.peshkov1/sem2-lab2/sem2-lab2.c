#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* thread_routine() {
    for (int i = 1; i <= 10; i++) {
        printf("Нить-потомок: строка %d\n", i);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int result;
    result = pthread_create(&thread, NULL, thread_routine, NULL);

    if (result != 0) {
        fprintf(stderr, "Ошибка при создании нити: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= 10; i++) {
        printf("Родительская нить: строка %d\n", i);
    }

    result = pthread_join(thread, NULL);
    if (result != 0) {
        fprintf(stderr, "Ошибка при ожидании нити: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}