#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* thread_func() {
    unsigned long i = 0;
    while (++i) {
        printf("String number %lu\n", i);
    }

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
