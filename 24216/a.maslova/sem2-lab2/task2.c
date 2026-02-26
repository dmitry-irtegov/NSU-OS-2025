#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* thread_func(void* arg) {
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "Child: %d\n", i + 1);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    
    if (pthread_create(&thread, NULL, thread_func, NULL)) {
        fprintf(stderr, "Thread creation error\n");
        return 1;
    }
    
    if (pthread_join(thread, NULL)) {
        fprintf(stderr, "Thread join error\n");
        return 1;
    }
    
    for (int i = 0; i < 10; i++) {
        fprintf(stderr, "Parent: %d\n", i + 1);
    }
    
    
    return 0;
}