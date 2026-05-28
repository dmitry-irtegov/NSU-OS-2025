#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

pthread_mutex_t m[3];
volatile int child_ready = 0;

void *print_child_lines(void *arg) {
    pthread_mutex_lock(&m[1]);
    child_ready = 1;

    int held = 1;
    int wait_for = 0;

    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&m[wait_for]);
        printf("Дочерняя нить: %d\n", i + 1);
        pthread_mutex_unlock(&m[held]);
        held = wait_for;
        wait_for = (wait_for + 2) % 3;
    }
    pthread_mutex_unlock(&m[held]);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    for (int i = 0; i < 3; i++) {
        pthread_mutex_init(&m[i], &attr);
    }
    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&m[0]);
    pthread_mutex_lock(&m[2]);

    if (pthread_create(&thread, NULL, print_child_lines, NULL) != 0) {
        perror("Ошибка при создании нити");
        return 1;
    }

    while (!child_ready) {
        sched_yield();
    }

    int unlock_idx = 0;
    int lock_idx = 1;

    for (int i = 0; i < 10; i++) {
        printf("Родительская нить: %d\n", i + 1);
        pthread_mutex_unlock(&m[unlock_idx]);
        pthread_mutex_lock(&m[lock_idx]);
        unlock_idx = (unlock_idx + 2) % 3;
        lock_idx = (lock_idx + 2) % 3;
    }

    if (pthread_join(thread, NULL) != 0) {
        perror("Ошибка при ожидании завершения нити");
        return 1;
    }

    pthread_mutex_unlock(&m[unlock_idx]);
    pthread_mutex_unlock(&m[(unlock_idx + 2) % 3]);

    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&m[i]);
    }

    return 0;
}
