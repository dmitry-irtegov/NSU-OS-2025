#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void thread_end() {
    printf("Thread terminated\n");
}

void* thread_func() {
    pthread_cleanup_push(thread_end, NULL);

    unsigned long i = 0;
    while (++i) {
        printf("String number %lu\n", i);
    }

    pthread_cleanup_pop(0);

    return NULL;
}

int main () {
    pthread_t thread;

    if (pthread_create(&thread, NULL, thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

    sleep(2);

    if (pthread_cancel(thread) != 0) {
        perror("pthread_cancel");
        exit(2);
    }

    //pthread_exit(NULL);
    exit(0);
}
