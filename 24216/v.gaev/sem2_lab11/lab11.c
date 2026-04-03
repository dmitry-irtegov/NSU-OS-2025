#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
int turn = 0;

void *child_task(void *arg) {
    (void)arg;

    for (int i = 1; i <= 10; i++) {
        pthread_mutex_lock(&mutex);

        while (turn != 1) {
            pthread_cond_wait(&cond, &mutex);
        }

        fprintf(stderr, "Дочерняя нить: строка %d\n", i);
        
        turn = 0;
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(void) {
    pthread_t thread;
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&cond, NULL);

    if (pthread_create(&thread, NULL, child_task, NULL) != 0) {
        perror("Ошибка создания нити");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= 10; i++) {
        pthread_mutex_lock(&mutex);

        while (turn != 0) {
            pthread_cond_wait(&cond, &mutex);
        }

        fprintf(stderr, "Родительская нить: строка %d\n", i);
        
        turn = 1;
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
    }

    pthread_join(thread, NULL);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    fprintf(stderr, "Главная программа: все нити завершили работу.\n");

    return 0;
}
