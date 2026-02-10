#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void *thread_run(void *arg) {
    for (int i = 0; i < 10; i++) {
        printf("I'm alive (child)\n");
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t pthread;

    int error = pthread_create(&pthread, NULL, thread_run, NULL);
    if (error) {
        fprintf(stderr, "could not create thread: %s\n", strerror(error));
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        printf("I'm alive (parent)\n");
    }
    pthread_exit(NULL);
}
