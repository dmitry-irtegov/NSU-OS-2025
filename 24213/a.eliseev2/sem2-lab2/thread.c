#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void *thread_run(void *arg) {
    for (int i = 0; i < 10; i++) {
        printf("I'm alive (child)\n");
    }
    return NULL;
}

int main() {
    pthread_t pthread;

    int create_error = pthread_create(&pthread, NULL, thread_run, NULL);
    if (create_error) {
        fprintf(stderr, "could not create thread: %s\n", strerror(create_error));
        return 1;
    }
    int join_error = pthread_join(pthread, NULL);
    if (join_error) {
        fprintf(stderr, "could not join thread: %s\n", strerror(join_error));
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        printf("I'm alive (parent)\n");
    }
    return 0;
}
