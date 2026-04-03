#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>

pthread_mutex_t m_parent;
pthread_mutex_t m_child;
pthread_mutex_t m_common;

void *child_task(void *arg) {
    (void)arg;

    pthread_mutex_lock(&m_child);

    for (int i = 1; i <= 10; i++) {
        pthread_mutex_lock(&m_parent);
        
        fprintf(stderr, "Дочерняя нить: строка %d\n", i);
        
        pthread_mutex_unlock(&m_child);
        pthread_mutex_lock(&m_common);
        pthread_mutex_unlock(&m_parent);
        pthread_mutex_lock(&m_child);
        pthread_mutex_unlock(&m_common);
    }

    pthread_mutex_unlock(&m_child);
    return NULL;
}

int main(void) {
    pthread_t thread;
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&m_parent, &attr);
    pthread_mutex_init(&m_child, &attr);
    pthread_mutex_init(&m_common, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&m_parent);
    
    if (pthread_create(&thread, NULL, child_task, NULL) != 0) {
        perror("Ошибка создания нити");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int err = pthread_mutex_trylock(&m_child);
        if (err == EBUSY) {
            break;
        }
        if (err == 0) {
            pthread_mutex_unlock(&m_child);
            sched_yield(); 
        }
    }

    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "Родительская нить: строка %d\n", i);

        pthread_mutex_lock(&m_common);
        pthread_mutex_unlock(&m_parent);
        pthread_mutex_lock(&m_child);
        pthread_mutex_unlock(&m_common);
        pthread_mutex_lock(&m_parent);
        pthread_mutex_unlock(&m_child);
    }

    pthread_mutex_unlock(&m_parent);
    
    pthread_join(thread, NULL);

    pthread_mutex_destroy(&m_parent);
    pthread_mutex_destroy(&m_child);
    pthread_mutex_destroy(&m_common);

    fprintf(stderr, "Главная программа: все нити завершили работу.\n");

    return 0;
}
