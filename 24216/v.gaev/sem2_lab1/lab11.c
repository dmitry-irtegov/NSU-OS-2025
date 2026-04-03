#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t m1, m2;

void* child_task(void* arg) {
    for (int i = 1; i <= 10; i++) {
        pthread_mutex_lock(&m2);
        fprintf(stderr, "Дочерняя нить: строка %d\n", i);
        pthread_mutex_unlock(&m1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t thread_id;
    pthread_mutexattr_t attr;
    int s;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&m1, &attr);
    pthread_mutex_init(&m2, &attr);

    pthread_mutex_lock(&m1);
    pthread_mutex_lock(&m2);

    s = pthread_create(&thread_id, NULL, child_task, NULL);
    if (s != 0) {
        fprintf(stderr, "Ошибка создания нити: %s\n", strerror(s));
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= 10; i++) {
        if (i > 1) {
            pthread_mutex_lock(&m1);
        }

        fprintf(stderr, "Родительская нить: строка %d\n", i);
        pthread_mutex_unlock(&m2);
    }

    s = pthread_join(thread_id, NULL);
    if (s != 0) {
        fprintf(stderr, "Ошибка присоединения нити: %s\n", strerror(s));
        exit(EXIT_FAILURE);
    }

    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m
