#include <pthread.h> 
#include <stdio.h> 
#include <string.h>

typedef struct connection {
    pthread_mutex_t *mtx;
    pthread_cond_t *cond; 
    int isFst; 
} connection; 

void * consumer(void *arg) {
    if (arg == NULL) {
        return ((void *) 0); 
    }
    connection *conn = (connection *)(arg); 
    for (int i = 1; i <= 10; ++i) {
        pthread_mutex_lock(conn->mtx);
        while (!(conn->isFst)) {
            pthread_cond_wait(conn->cond, conn->mtx);
        }
        printf("Thread %d: string %d\n", 2, i);
        conn->isFst = 0; 
        pthread_mutex_unlock(conn->mtx); 
        pthread_cond_signal(conn->cond); 
    }
    return ((void *) 0); 
}

int main() {

    connection conn = {NULL, NULL, 0};
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; 
    conn.mtx = &mtx; 
    pthread_cond_t cond; 
    int err = pthread_cond_init(&cond, NULL); 
    if (pthread_cond_init(&cond, NULL) == -1) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! COND-INIT %s", buff); 
        return 0; 
    }
    conn.cond = &cond; 
    pthread_t cthd;
    err = pthread_create(&cthd, NULL, consumer, &conn);
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! CREATE %s", buff); 
        return 0;  
    }

    for (int i = 1; i <= 10; ++i) {
        pthread_mutex_lock(conn.mtx);
        while (conn.isFst) {
            pthread_cond_wait(conn.cond, conn.mtx);
        }
        printf("Thread %d: string %d\n", 1, i);
        conn.isFst = 1; 
        pthread_mutex_unlock(conn.mtx); 
        pthread_cond_signal(conn.cond); 
    }

    err = pthread_join(cthd, NULL);
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! MTX-JOIN %s", buff); 
        return 0;   
    }

    err = pthread_mutex_destroy(conn.mtx); 
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! MTX-DESTROY %s", buff); 
        return 0; 
    }

    err = pthread_cond_destroy(conn.cond); 
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! COND-DESTROY %s", buff); 
        return 0; 
    }
    
}