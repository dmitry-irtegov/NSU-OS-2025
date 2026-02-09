#include <pthread.h>
#include <stdio.h> 
#include <unistd.h>

void * InftyPrint(void *arg) {
    while (1) {
        printf("String\n");
    }
    return ((void *)1); 
}

int main() {
    pthread_t tid; 
    int err = pthread_create(&tid, NULL, InftyPrint, NULL);
    if (err != 0) {
        printf("Cannot create a thread\n");
        return 0; 
    }
    sleep(2); 
    err = pthread_cancel(tid); 
    if (err != 0) {
        printf("Cannot cancel the thread\n");
        return 0; 
    }
    err = pthread_join(tid, NULL); 
    if (err != 0) {
        printf("Cannot wait for the second thread\n");
        return 0;  
    }
    printf("Thread 2 interrupted\n");
    return 0;  
}