#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t m1, m2;

void* child_task(void* arg) {
    pthread_mutex_lock(&m2);
    pthread_mutex_lock(&m1);

    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "Дочерняя нить: строка %d\n", i);
        if (i % 2 != 0) { 
            pthread_mutex_unlock(&m2);
            if (i < 10) pthread_mutex_lock(&m2);
        } else {       
            pthread_mutex_unlock(&m1);
            if (i < 10) pthread_mutex_lock(&m1);
        }
    }
    pthread_mutex_unlock(&m2);
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

    s = pthread_create(&thread_id, NULL, child_task, NULL);
    if (s != 0) {
        fprintf(stderr, "Ошибка создания нити: %s\n", strerror(s));
        exit(EXIT_FAILURE);
    }

    sleep(1);

    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "Родительская нить: строка %d\n", i);
        if (i % 2 != 0) {
            pthread_mutex_unlock(&m1);
            if (i < 10) pthread_mutex_lock(&m2);
        } else {
            pthread_mutex_unlock(&m2);
            if (i < 10) pthread_mutex_lock(&m1);
        }
    }

    pthread_mutex_unlock(&m1);

    s = pthread_join(thread_id, NULL);
    if (s != 0) {
        fprintf(stderr, "Ошибка присоединения нити: %s\n", strerror(s));
        exit(EXIT_FAILURE);
    }

    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);
    pthread_mutexattr_destroy(&attr);

    fprintf(stderr, "Главная программа: все нити завершили работу.\n");

    return 0;
}
