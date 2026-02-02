#include <pthread.h>
#include <stdio.h>

void* child_function(void* arg) {
    for (int i = 0; i < 10; i++) {
        printf("Child thread: %d\n", i);
    }
    
    return NULL;
}

int main() {
    pthread_t thread_id;
    int result;

    result = pthread_create(&thread_id, NULL, child_function, NULL);

    if (result != 0) {
        perror("Thread creation error");
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        printf("Parent thread %d\n", i + 1);
    }

    return 0;
}