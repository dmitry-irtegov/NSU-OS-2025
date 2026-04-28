#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t m[3];

void* child_thread(void* arg) {
    int n = *(int*)arg;

    pthread_mutex_lock(&m[1]);
    pthread_mutex_lock(&m[2]);

    for (int i = 0; i < n; i++) {
        pthread_mutex_unlock(&m[(i + 1) % 3]);
        pthread_mutex_lock(&m[i % 3]);
        fprintf(stderr, "Child thread: %d\n", i + 1);
    }

    pthread_mutex_unlock(&m[0]);
    pthread_mutex_unlock(&m[2]);

    return NULL;
}

int main(void) {
    int n = 10;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    for (int i = 0; i < 3; i++) {
        if (pthread_mutex_init(&m[i], &attr) != 0) {
            fprintf(stderr, "mutex init error\n");
            return 1;
        }
    }
    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&m[0]);
    pthread_mutex_lock(&m[2]);

    pthread_t tid;
    if (pthread_create(&tid, NULL, child_thread, &n) != 0) {
        perror("pthread_create");
        return 1;
    }

    usleep(100000);

    for (int i = 0; i < n; i++) {
        pthread_mutex_unlock(&m[(i + 2) % 3]);
        pthread_mutex_lock(&m[(i + 1) % 3]);
        fprintf(stderr, "Parent thread: %d\n", i + 1);
    }

    pthread_mutex_unlock(&m[0]);
    pthread_mutex_unlock(&m[1]);

    pthread_join(tid, NULL);

    for (int i = 0; i < 3; i++)
        pthread_mutex_destroy(&m[i]);

    return 0;
}

