#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t m1;
pthread_mutex_t m2;

void* print_10_lines(void *arg) {
    (void)arg;
    pthread_mutex_t *my = &m2;
    pthread_mutex_t *other = &m1;
    pthread_mutex_t *tmp;

    if (pthread_mutex_lock(my) != 0) {
        perror("child lock init");
        return NULL;
    }

    for (int i = 0; i < 10; i++) {
        if (pthread_mutex_lock(other) != 0) {
            perror("child lock other");
            return NULL;
        }

        fprintf(stderr, "new thread\n");
        
        if (pthread_mutex_unlock(my) != 0) {
            perror("child unlock my");
            return NULL;
        }
        
        usleep(10);
        
        tmp = my;
        my = other;
        other = tmp;
    }
    
    pthread_mutex_unlock(my);

    return NULL;
}

int main() {
    pthread_mutexattr_t m_attr;
    pthread_mutexattr_init(&m_attr);
    pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_ERRORCHECK);
    
    if (pthread_mutex_init(&m1, &m_attr) != 0 || pthread_mutex_init(&m2, &m_attr) != 0) {
        perror("mutex init");
        exit(EXIT_FAILURE);
    }
    pthread_mutexattr_destroy(&m_attr);

    if (pthread_mutex_lock(&m1) != 0) {
        perror("parent lock init");
        exit(EXIT_FAILURE);
    }

    pthread_t thread;
    int result = pthread_create(&thread, NULL, print_10_lines, NULL);
    if (result != 0) {
        fprintf(stderr, "Error creating thread: %s\n", strerror(result));
        exit(EXIT_FAILURE);
    }

    usleep(10000); 

    pthread_mutex_t *my = &m1;
    pthread_mutex_t *other = &m2;
    pthread_mutex_t *tmp;

    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "main thread\n");
        
        if (pthread_mutex_unlock(my) != 0) {
             perror("parent unlock my");
             exit(EXIT_FAILURE);
        }
        
        usleep(10);

        if (pthread_mutex_lock(other) != 0) {
             perror("parent lock other");
             exit(EXIT_FAILURE);
        }

        tmp = my;
        my = other;
        other = tmp;
    }

    pthread_mutex_unlock(my);

    pthread_join(thread, NULL); 
    
    pthread_mutex_destroy(&m1);
    pthread_mutex_destroy(&m2);

    pthread_exit(NULL);
}
