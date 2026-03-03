#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void cleanup_handler(void *arg) {
    printf("Thread is cleaning up before cancel...\n");
}

void* worker(void* arg) {
    pthread_cleanup_push(cleanup_handler, NULL);

    while (1) {
        printf("Thread is working\n");
    }

    pthread_cleanup_pop(0);
    
    return NULL;
}

int main() {
    pthread_t th;

    int code = pthread_create(&th, NULL, worker, NULL);
    if (code != 0) {
        fprintf(stderr, "creating thread: %s\n", strerror(code));
        return -1;
    }
    
    sleep(1);
    code = pthread_cancel(th);
    if (code != 0) {
        fprintf(stderr, "cancel thread: %s\n", strerror(code));
        return -1;
    }
    
    code = pthread_join(th, NULL);
    if (code != 0) {
        fprintf(stderr, "join thread: %s\n", strerror(code));
        return -1;
    }

    printf("Thread joined\n");

    return 0;
}
