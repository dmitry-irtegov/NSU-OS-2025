#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t m1, m2;

void* child_task(void* arg) {
    (void)arg; 

    pthread_mutex_t *my_m = &m2;
    pthread_mutex_t *other_m = &m1;

    pthread_mutex_lock(my_m);

    for (int i = 1; i <= 10; i++) {
        pthread_mutex_lock(other_m);
        
        fprintf(stderr, "Дочерняя нить: строка %d\n", i);
        
        pthread_mutex_unlock(my_m);

        pthread_mutex_t *temp = my_m;
        my_m = other_m;
        other_m = temp;
    }

    pthread_mutex_unlock(my_m);
    return NULL;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    pthread_t thread_id;
    pthread_mutexattr_t attr;
    int s;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&m1, &attr);
    pthread_mutex_init(&m2, &attr);

    pthread_mutex_t *my_m = &m1;
    pthread_mutex_t *other_m = &m2;

    pthread_mutex_lock(my_m);

    s = pthread_create(&thread_id, NULL, child_task, NULL);
    if (s != 0) {
        fprintf(stderr, "Ошибка создания нити: %s\n", strerror(s));
        exit(EXIT_FAILURE);
    }

    sleep(1);

    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "Родительская нить: строка %d\n", i);
        
        pthread_mutex_unlock(my_m);
        pthread_mutex_lock(other_m);

        pthread_mutex_t *temp = my_m;
        my_m = other_m;
        other_m = temp;
    }

    pthread_mutex_unlock(my_m);

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
