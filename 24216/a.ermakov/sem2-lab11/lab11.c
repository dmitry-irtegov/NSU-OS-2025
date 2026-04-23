#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pthread_mutex_t mx_first;
pthread_mutex_t mx_second;
pthread_mutex_t mx_third;

atomic_int child_is_ready = 0;

pthread_mutex_t* mutex_by_slot(int slot) {
    switch (slot % 3) {
        case 0:
            return &mx_first;
        case 1:
            return &mx_second;
        default:
            return &mx_third;
    }
}

void print_sequence(const char* title, int lock_shift, int unlock_shift) {
    for (int line = 1; line <= 10; line++) {
        int step = line - 1;
        pthread_mutex_lock(mutex_by_slot(step + lock_shift));

        fprintf(stderr, "%s %d\n", title, line);

        pthread_mutex_unlock(mutex_by_slot(step + unlock_shift));
    }

    pthread_mutex_unlock(mutex_by_slot(9 + lock_shift));
}

void* child_worker(void* arg) {
    pthread_mutex_lock(&mx_third);

    atomic_store(&child_is_ready, 1);

    print_sequence((const char*)arg, 0, 2);

    pthread_exit(NULL);
}

int main() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    if (pthread_mutex_init(&mx_first, &attr) != 0) {
        fprintf(stderr, "Ошибка инициализации первого мьютекса\n");
        return 1;
    }
    if (pthread_mutex_init(&mx_second, &attr) != 0) {
        fprintf(stderr, "Ошибка инициализации второго мьютекса\n");
        return 1;
    }
    if (pthread_mutex_init(&mx_third, &attr) != 0) {
        fprintf(stderr, "Ошибка инициализации третьего мьютекса\n");
        return 1;
    }

    pthread_mutexattr_destroy(&attr);
    pthread_mutex_lock(&mx_first);

    pthread_t thread;
    int error = pthread_create(&thread, NULL, child_worker, "Дочерняя нить:");
    if (error != 0) {
        fprintf(stderr, "Ошибка создания потока: %s\n", strerror(error));
        return 1;
    }

    while (!atomic_load(&child_is_ready)) {
        sched_yield();
    }

    print_sequence("Родительская нить:", 1, 0);

    pthread_join(thread, NULL);

    pthread_mutex_destroy(&mx_first);
    pthread_mutex_destroy(&mx_second);
    pthread_mutex_destroy(&mx_third);

    return 0;
}
