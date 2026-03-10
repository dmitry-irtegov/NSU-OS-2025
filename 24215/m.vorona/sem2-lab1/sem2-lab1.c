#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *thread_func(void *arg) {
    (void)arg;
    for (int i = 1; i <= 10; i++) {
        printf("child  %d\n", i);
    }
    return NULL;
}

int main() {
    pthread_t tid;
    int err;

    err = pthread_create(&tid, NULL, thread_func, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        return 1;
    }

    for (int i = 1; i <= 10; i++) {
        printf("parent %d\n", i);
    }

    err = pthread_join(tid, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(err));
        return 1;
    }

    return 0;
}
