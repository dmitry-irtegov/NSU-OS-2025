#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* child_task(void* arg) {
    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "Дочерняя нить: строка %d\n", i);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t thread_id;
    int s;

    s = pthread_create(&thread_id, NULL, child_task, NULL);
    if (s != 0) {
        fprintf(stderr, "Ошибка создания нити: %s\n", strerror(s));
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "Родительская нить: строка %d\n", i);
    }

    s = pthread_join(thread_id, NULL);
    if (s != 0) {
        fprintf(stderr, "Ошибка присоединения нити: %s\n", strerror(s));
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Главная программа: все нити завершили работу.\n");

    return 0;
}
