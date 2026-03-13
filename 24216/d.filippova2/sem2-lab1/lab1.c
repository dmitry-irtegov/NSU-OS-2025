#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void* printLines(void *arg) {
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "line\n");
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int error = pthread_create(&thread, NULL, printLines, NULL);
    if (error != 0) {
        fprintf(stderr, "Error creating thread: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "main line\n");
    }
    pthread_exit(NULL);
}