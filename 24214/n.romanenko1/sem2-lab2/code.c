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

    if (pthread_create(&thread_id, NULL, child_routine, NULL) != 0) {
        perror("Thread creation error");
        return 1;
    }

    pthread_join(thread_id, NULL); 

    for (int i = 0; i < 10; i++) {
        printf("Parent thread %d\n", i + 1);
    }

    return 0;
}