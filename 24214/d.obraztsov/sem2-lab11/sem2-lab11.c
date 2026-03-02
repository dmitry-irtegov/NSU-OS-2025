#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define LINES_COUNT 10

pthread_mutex_t m[3];

void* thread_body(void* arg) {
    for (int i = 0; i < LINES_COUNT; i++) {
        pthread_mutex_lock(&m[2]); 
        pthread_mutex_lock(&m[1]);
        
        printf("Child thread: line %d\n", i + 1);
        
        pthread_mutex_unlock(&m[1]);
        pthread_mutex_unlock(&m[0]);
    }
    return NULL;
}

int main() {
    pthread_t thread;

    for (int i = 0; i < 3; i++) {
        pthread_mutex_init(&m[i], NULL);
    }

    pthread_mutex_lock(&m[0]); 
    pthread_mutex_lock(&m[1]); 

    if (pthread_create(&thread, NULL, thread_body, NULL) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < LINES_COUNT; i++) {
        if (i > 0) pthread_mutex_lock(&m[0]);
        
        printf("Parent thread: line %d\n", i + 1);

        pthread_mutex_unlock(&m[1]);
        pthread_mutex_unlock(&m[2]);
    }

    pthread_join(thread, NULL);

    for (int i = 0; i < 3; i++) {
        pthread_mutex_destroy(&m[i]);
    }

    return EXIT_SUCCESS;
}