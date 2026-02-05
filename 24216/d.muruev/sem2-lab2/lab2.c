#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void* print_10_lines(void *arg) {
    for (int i = 0; i < 10; i++) {
        printf("new thread\n");
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int result;
    result = pthread_create(&thread, NULL, print_10_lines, NULL);
    if (result != 0) {
        fprintf(stderr, "Error creating thread: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }

    result = pthread_join(thread, NULL);
    if (result != 0) {
        fprintf(stderr, "Error join: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 10; i++) {
        printf("main thread\n");
    }

    return 0;
}
