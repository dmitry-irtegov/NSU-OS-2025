#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void* thread_body(void* param) {
    (void)param;

    while (1) {
        printf("child\n");
    }

    return NULL;
}

int main() {
    pthread_t thread;
    void* res;

    if (pthread_create(&thread, NULL, thread_body, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    sleep(2);

    if (pthread_cancel(thread) != 0) {
        perror("pthread_cancel");
        return 1;
    }

    pthread_join(thread, &res);

    if (res == PTHREAD_CANCELED) {
        printf("\nCancelled\n");
    }

    return 0;
}