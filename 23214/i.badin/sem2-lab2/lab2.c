#include <stdio.h>
#include <pthread.h>
#include <string.h>

void *thread_function(void *arg) {
    for (int i = 0; i < 10; ++i) {
        printf("child %d\n", i + 1);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int errnum;

    errnum = pthread_create(&thread, NULL, thread_function, NULL);
    if (errnum != 0) {
        fprintf(stderr, "error while creating thread: %s\n", strerror(errnum));
        return 1;
    }

    errnum = pthread_join(thread, NULL);
    if (errnum != 0) {
        fprintf(stderr, "error while joining thread: %s\n", strerror(errnum));
        return 1;
    }

    for (int i = 0; i < 10; ++i) {
        printf("parent %d\n", i + 1);
    }

    return 0;
}
