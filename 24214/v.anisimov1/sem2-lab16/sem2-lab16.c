#include <semaphore.h> 
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h> 
#include <stdio.h> 
#include <sys/wait.h> 
#include <sys/fcntl.h> 

typedef struct connection {
    sem_t *producer; 
    sem_t *consumer;
    char *producerName; 
    char *consumerName; 
} connection; 

int initConnection(connection *conn) {
    sem_unlink(conn->producerName);
    sem_unlink(conn->consumerName);

    int code = 0; 

    conn->producer = sem_open(conn->producerName, O_CREAT, 0700, 0); 
    if (conn->producer == SEM_FAILED) {
        code = -1; 
    }

    conn->consumer = sem_open(conn->consumerName, O_CREAT, 0700, 1); 
    if (conn->consumer == SEM_FAILED) {
        code = -1;
    }

    return code; 
}

int destroyConnection(connection *conn) {
    int code = 0; 

    if (conn->producer != SEM_FAILED) {
        if (sem_close(conn->producer) != 0) 
            code = -1;

        if (sem_unlink(conn->producerName) != 0) 
            code = -1;
    }

    if (conn->consumer != SEM_FAILED) {
        if (sem_close(conn->consumer) != 0) 
            code = -1;

        if (sem_unlink(conn->consumerName) != 0) 
            code = -1;
    }

    return code; 
}

int main() {
    connection conn = {0};
    
    conn.producerName = "/producer";
    conn.consumerName = "/consumer";

    if (initConnection(&conn) != 0) {
        perror("[ERROR] initConnection");
        destroyConnection(&conn); 
        return 0; 
    }

    pid_t consumer = fork(); 
    if (consumer == -1) {
        perror("[ERROR] - fork"); 
        destroyConnection(&conn); 
        return 0;
    }

    if (consumer != 0) {
        for (int i = 1; i <= 10; ++i) {
            sem_wait(conn.consumer);
            printf("Parent process: %d\n", i);
            sem_post(conn.producer);
        }
    } else {
        for (int i = 1; i <= 10; ++i) {
            sem_wait(conn.producer);
            printf("Child process: %d\n", i);
            sem_post(conn.consumer); 
        }
        exit(0);
    }

    pid_t child = wait(NULL);
    if (child == -1)
        perror("[ERROR] wait");

    printf("[APP]: consumer finished its work\n");

    if (destroyConnection(&conn) != 0) {
        perror("[ERROR] destroyConnection");
    }  

    printf("[APP]: connection has been destroyed\n");
    
    return 0; 
}