#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void* print_10_lines(void *arg) {
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "new thread\n");
    }
    return NULL;
}

int main() {
    pthread_t thread;
    int result = pthread_create(&thread, NULL, print_10_lines, NULL);
    if (result != 0) {
        fprintf(stderr, "Error creating thread: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }
    
    int join_result = pthread_join(thread, NULL);
    if (join_result != 0) {
        fprintf(stderr, "Error joining thread: %s\n", strerror(join_result));
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "main thread\n");
    }
    return 0;
}
