#include <pthread.h>
#include <stdio.h> 
#include <unistd.h>
#include <string.h> 

void message(void *arg) {
    printf("I was interrupted...\n");
}

void * InftyPrint(void *arg) {
    pthread_cleanup_push(message, NULL);
    while (1) {
        printf("String\n");
    }
    pthread_cleanup_pop(0);
    return ((void *)1);
}

int main() { 
    pthread_t tid; 
    int err = pthread_create(&tid, NULL, InftyPrint, NULL);
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! Creation %s", buff); 
        return 0; 
    }
    sleep(2); 
    err = pthread_cancel(tid); 
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! Cancel %s", buff); 
        return 0;
    }
    err = pthread_join(tid, NULL); 
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! Join %s", buff); 
        return 0;  
    }
    return 0;  
}