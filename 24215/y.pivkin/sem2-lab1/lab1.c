#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* thread_func() {
    for (int i = 0; i < 10; i++) {
        printf("Child thread %d\n", i + 1);
    }

    return NULL;
}

int main () {
    pthread_t thread;

    if (pthread_create(&thread, NULL, thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

    for (int i = 0; i < 10; i++) {
        printf("Parent thread %d\n", i + 1);
    }

    pthread_exit(NULL);
    exit(0);
}
