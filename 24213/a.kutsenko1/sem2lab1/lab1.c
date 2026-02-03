#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

void* ten_function(void* arg) {
    for (int i = 1; i <= 10; i++) {
        printf("Child thread: line %d\n", i);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    if (pthread_create(&thread, NULL, ten_function, NULL) != 0) {
        fprintf(stderr, "Thread creation error\n");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 1; i <= 10; i++) {
        printf("Parent thread: line %d\n", i);
    }
    
    pthread_exit(NULL);
}
