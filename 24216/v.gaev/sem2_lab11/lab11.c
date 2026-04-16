#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>

pthread_mutex_t m[3];
atomic_int child_ready = 0;

void *child_task(void *arg) {
    (void)arg;

    pthread_mutex_lock(&m[2]);
    atomic_store(&child_ready, 1);

    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&m[i % 3]); 
        fprintf(stderr, "Дочерняя нить: строка %d\n", i + 1);
        pthread_mutex_unlock(&m[(i + 2) % 3]);
    }
    
    pthread_mutex_unlock(&m[0]); 

    return NULL;
}

int main(void) {
    pthread_t thread;
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    for (int i = 0; i < 3; i++) {
        if (pthread_mutex_init(&m[i], &attr) != 0) {
            perror("Ошибка инициализации мьютекса");
            exit(EXIT_FAILURE);
        }
    }
    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&m[0]);

    if (pthread_create(&thread, NULL, child_task, NULL) != 0) {
        perror("Ошибка создания нити");
        exit(EXIT_FAILURE);
    }

    while (!atomic_load(&child_ready)) {
        sched_yield(); 
    }
    
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&m[(i + 1) % 3]);
        fprintf(stderr, "Родительская нить: строка %d\n", i + 1);
        pthread_mutex_unlock(&m[i % 3]); 
    }
    
    pthread_mutex_unlock(&m[1 % 3]);

    pthread_join(thread, NULL);

    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&m[i]);
    }

    fprintf(stderr, "Главная программа: все нити завершили работу.\n");

    return 0;
}
