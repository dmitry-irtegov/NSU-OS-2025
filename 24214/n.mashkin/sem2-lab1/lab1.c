#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_LINES 10

void* child() {
    for (int i = 0; i < NUM_LINES; i++) {
        printf("Child: %d\n", i);
    }
    
    return NULL;
}

int main() {
    pthread_t thread;
    
    if (pthread_create(&thread, NULL, child, NULL) != 0) {
        fprintf(stderr, "thread creation error");
        return 1;
    }

    for (int i = 0; i < NUM_LINES; i++) {
        printf("Parent: %d\n", i);
    }
    
    pthread_join(thread, NULL);
    return 0;
}
