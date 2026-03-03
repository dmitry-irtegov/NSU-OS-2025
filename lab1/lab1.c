#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


void* thread_function(void* arg) {
    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "Child line %d\n", i);
    }
    return NULL;
}

int main() {
    pthread_t thread;

    int result = pthread_create(&thread, NULL, thread_function, NULL);
    if (result != 0) {
        char buf[256];
        strerror_r(result, buf, sizeof(buf));
        fprintf(stderr, "creating thread: %s\n", buf);
        return EXIT_FAILURE;
    }

    for (int i = 1; i <= 10; i++) {
        fprintf(stderr, "Parent line %d\n", i);
    }

    int join_result = pthread_join(thread, NULL);
    if (join_result != 0) {
        char buf[256];
        strerror_r(join_result, buf, sizeof(buf));
        fprintf(stderr, "Error joining: %s\n", buf);
        return EXIT_FAILURE;
    }
    

    return EXIT_SUCCESS;
}