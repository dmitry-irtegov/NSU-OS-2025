#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define LINES_COUNT 10

void* thread_body(void* arg) {
    for (int i = 0; i < LINES_COUNT; i++) {
        printf("Child thread: line %d\n", i + 1);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int result;

    result = pthread_create(&thread, NULL, thread_body, NULL);

    if (result != 0) {
        fprintf(stderr, "Error creating thread: %s\n", strerror(result));
        return EXIT_FAILURE;
    }

    for (int i = 0; i < LINES_COUNT; i++) {
        printf("Parent thread: line %d\n", i + 1);
    }

    result = pthread_join(thread, NULL);
    if (result != 0) {
        fprintf(stderr, "Error joining thread: %s\n", strerror(result));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}