#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t parent_mutex;
pthread_mutex_t child_mutex;
pthread_mutex_t sync_mutex;

void release_to_child() {
    pthread_mutex_unlock(&child_mutex);
    pthread_mutex_lock(&sync_mutex);
    pthread_mutex_unlock(&parent_mutex);
}

void release_to_parent() {
    pthread_mutex_lock(&child_mutex);
    pthread_mutex_unlock(&sync_mutex);
    pthread_mutex_lock(&parent_mutex);
}

void* print_10_lines(void *arg) {
    (void)arg;
    pthread_mutex_lock(&sync_mutex);
    for (int i = 0; i < 10; i++) {
        release_to_parent();
        fprintf(stderr, "new thread\n");
        release_to_child();
    }
    pthread_mutex_unlock(&sync_mutex);
    return NULL;
}

int main() {
    pthread_t thread;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&parent_mutex, &attr);
    pthread_mutex_init(&child_mutex, &attr);
    pthread_mutex_init(&sync_mutex, &attr);

    pthread_mutex_lock(&parent_mutex);
    pthread_mutex_lock(&child_mutex);

    int result = pthread_create(&thread, NULL, print_10_lines, NULL);
    if (result != 0) {
        fprintf(stderr, "Error creating thread: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }

    while (pthread_mutex_trylock(&sync_mutex) == 0) {
        pthread_mutex_unlock(&sync_mutex);  
    }

    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "main thread\n");
        release_to_child();
        release_to_parent();
    }

    pthread_join(thread, NULL);

    pthread_mutex_destroy(&parent_mutex);
    pthread_mutex_destroy(&child_mutex);
    pthread_mutex_destroy(&sync_mutex);
    
    pthread_mutexattr_destroy(&attr);

    return EXIT_SUCCESS;
}
