#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pthread_mutex_t sync_mutex;
pthread_cond_t turn_cond;
int turn = 0;

void mutex_lock(pthread_mutex_t* mutex) {
    int error = pthread_mutex_lock(mutex);
    if (error != 0) {
        fprintf(stderr, "Error locking mutex: %s\n", strerror(error));
        exit(1);
    }
}

void mutex_unlock(pthread_mutex_t* mutex) {
    int error = pthread_mutex_unlock(mutex);
    if (error != 0) {
        fprintf(stderr, "Error unlocking mutex: %s\n", strerror(error));
        exit(1);
    }
}

void cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    int error = pthread_cond_wait(cond, mutex);
    if (error != 0) {
        fprintf(stderr, "Error waiting on cond: %s\n", strerror(error));
        exit(1);
    }
}

void cond_signal(pthread_cond_t* cond) {
    int error = pthread_cond_signal(cond);
    if (error != 0) {
        fprintf(stderr, "Error signaling cond: %s\n", strerror(error));
        exit(1);
    }
}

void* printer_ten(void* arg) {
    (void)arg;
    for (int i = 0; i < 10; i++) {
        mutex_lock(&sync_mutex);
        while (turn != 1) {
            cond_wait(&turn_cond, &sync_mutex);
        }
        fprintf(stderr, "Child string: %d\n", i + 1);
        turn = 0;

        mutex_unlock(&sync_mutex);
        cond_signal(&turn_cond);
    }

    return NULL;
}

int main() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    int error = pthread_mutex_init(&sync_mutex, &attr);
    if (error != 0) {
        fprintf(stderr, "Error initializing mutex: %s\n", strerror(error));
        return 1;
    }
    pthread_mutexattr_destroy(&attr);

    error = pthread_cond_init(&turn_cond, NULL);
    if (error != 0) {
        fprintf(stderr, "Error initializing cond: %s\n", strerror(error));
        return 1;
    }

    pthread_t thread;
    error = pthread_create(&thread, NULL, printer_ten, NULL);
    if (error != 0) {
        fprintf(stderr, "Error creating threads: %s\n", strerror(error));
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        mutex_lock(&sync_mutex);
        while (turn != 0) {
            cond_wait(&turn_cond, &sync_mutex);
        }
        fprintf(stderr, "Parent string: %d\n", i + 1);
        turn = 1;

        mutex_unlock(&sync_mutex);
        cond_signal(&turn_cond);
    }

    error = pthread_join(thread, NULL);
    if (error != 0) {
        fprintf(stderr, "Error joining thread: %s\n", strerror(error));
        return 1;
    }

    pthread_mutex_destroy(&sync_mutex);
    pthread_cond_destroy(&turn_cond);

    return 0;
}
