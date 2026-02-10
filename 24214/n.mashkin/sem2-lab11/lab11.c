#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_LINES 10

pthread_mutex_t m_sync;
pthread_mutex_t m_parent;
pthread_mutex_t m_child;

void* child() {
    pthread_mutex_lock(&m_child);

    for (int i = 0; i < NUM_LINES; i++) {
        pthread_mutex_lock(&m_parent);
        pthread_mutex_unlock(&m_child);

        printf("Child: %d\n", i);

        pthread_mutex_lock(&m_sync);
        pthread_mutex_unlock(&m_parent);

        pthread_mutex_lock(&m_child);
        pthread_mutex_unlock(&m_sync);
    }
    
    return NULL;
}

int main() {
    pthread_t thread;
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    
    if (pthread_mutex_init(&m_sync, &attr) != 0 ||
        pthread_mutex_init(&m_parent, &attr) != 0 ||
        pthread_mutex_init(&m_child, &attr) != 0) {
        fprintf(stderr, "mutex initialization error\n");
        return 1;
    }
    
    pthread_mutex_lock(&m_parent);
    
    if (pthread_create(&thread, NULL, child, NULL) != 0) {
        fprintf(stderr, "thread creation error");
        return 1;
    }

    usleep(1000);
    
    for (int i = 0; i < NUM_LINES; i++) {
        printf("Parent: %d\n", i);

        pthread_mutex_lock(&m_sync);
        pthread_mutex_unlock(&m_parent);

        pthread_mutex_lock(&m_child);
        pthread_mutex_unlock(&m_sync);

        pthread_mutex_lock(&m_parent);
        pthread_mutex_unlock(&m_child);
    }
    
    pthread_join(thread, NULL);
    
    pthread_mutex_destroy(&m_child);
    pthread_mutex_destroy(&m_parent);
    pthread_mutex_destroy(&m_sync);
    pthread_mutexattr_destroy(&attr);
    return 0;
}
