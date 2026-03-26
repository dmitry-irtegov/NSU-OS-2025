#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pthread_mutex_t m[3];

atomic_int child_start = 0;

void print_lines(const char* str, int lock_offset, int unlock_offset) {
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&m[(i + lock_offset) % 3]);

        fprintf(stderr, "%s %d\n", str, i + 1);

        pthread_mutex_unlock(&m[(i + unlock_offset) % 3]);
    }

    pthread_mutex_unlock(&m[(9 + lock_offset) % 3]);
}

void* printer_ten(void* arg) {
    pthread_mutex_lock(&m[2]);

    atomic_store(&child_start, 1);

    print_lines((char*)arg, 0, 2);

    pthread_exit(NULL);
}

int main() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    for (int i = 0; i < 3; i++) {
        if (pthread_mutex_init(&m[i], &attr) != 0) {
            fprintf(stderr, "Error initializing mutex %d\n", i);
            return 1;
        }
    }
    pthread_mutexattr_destroy(&attr);
    pthread_mutex_lock(&m[0]);

    pthread_t thread;
    int error = pthread_create(&thread, NULL, printer_ten, "Child string:");
    if (error != 0) {
        fprintf(stderr, "Error creating threads: %s\n", strerror(error));
        return 1;
    }

    while (!atomic_load(&child_start)) {
        sched_yield();
    }

    print_lines("Parent string:", 1, 0);

    pthread_join(thread, NULL);

    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&m[i]);
    }

    return 0;
}