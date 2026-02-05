#include "pthread.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>

void* thread_printer(void *arg) {
    for (int i = 0; i < 10; i++) {
        printf("Child thread: %d\n", i);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int result = 0;

    result = pthread_create(&thread, NULL, thread_printer, NULL);
    if (result != 0) {
        fprintf(stderr, "Error pthread_create: %s\n", strerror(result));
    }

    for (int i = 0; i < 10; i++) {
        printf("Main thread: %d\n", i);
    }

    result = pthread_join(thread, NULL);
    if (result != 0) {
        fprintf(stderr, "Error pthread_join: %s\n", strerror(result));
    }

    return 0;
}