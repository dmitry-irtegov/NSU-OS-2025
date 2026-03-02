#include <pthread.h> 
#include <semaphore.h> 
#include <stdio.h> 
#include <string.h>

typedef struct connection {
    sem_t fst;
    sem_t snd; 
} connection; 

void * consumer(void *arg) {
    if (arg == NULL) {
        return ((void *) 1); 
    }
    connection *conn = (connection *)(arg); 
    for (int i = 1; i <= 10; ++i) {
        sem_wait(&(conn->fst)); 
        printf("Thread %d: string %d\n", 2, i);
        sem_post(&(conn->snd)); 
    }
    return((void *)1); 
}


int main() {
    pthread_t cthd;
    connection conn;
    if (sem_init(&conn.fst, 0, 0) == -1) {
        perror("[ERROR] Semaphore initialization");
        return 0; 
    }
    if (sem_init(&conn.snd, 0, 0) == -1) {
        perror("[ERROR] Semaphore initialization");
        return 0; 
    }
    sem_post(&conn.snd);
    int err = pthread_create(&cthd, NULL, consumer, &conn);
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! Creation %s", buff); 
        return 0;   
    }
    for (int i = 1; i <= 10; ++i) {
        sem_wait(&(conn.snd)); 
        printf("Thread %d: string %d\n", 1, i);
        sem_post(&(conn.fst)); 
    }
    err = pthread_join(cthd, NULL);
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! Join %s", buff); 
        return 0;  
    }
    if (sem_destroy(&(conn.fst)) != 0) {
        perror("[ERROR] Semaphore destroy");
        return 0; 
    }
    if (sem_destroy(&(conn.snd)) != 0) {
        perror("[ERROR] Semaphore destroy");
        return 0; 
    }
    return 0; 
}