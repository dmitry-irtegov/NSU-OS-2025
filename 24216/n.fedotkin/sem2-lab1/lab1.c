#include "pthread.h"
#include "stdio.h"

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
        perror("Error: pthread_create");
    }

    for (int i = 0; i < 10; i++) {
        printf("Main thread: %d\n", i);
    }

    result = pthread_join(thread, NULL);
    if (result != 0) {
        perror("Error: pthread_join");
    }

    return 0;
}