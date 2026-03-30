#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "queue.h"

Queue queue;

void* producer(void* arg) {
    int id = *(int*)arg;
    int counter = 1;
    char temp_buf[128];

    while (1) {
        snprintf(temp_buf, sizeof(temp_buf), "----------------------------Message %d from Producer %d----------------------12345", counter++, id);

        int bytes = mymsgput(&queue, temp_buf);
        if (bytes == 0) {
            printf("[Producer %d] shutting down\n", id);
            break;
        }

        printf("[Producer %d] Sent the first %d bytes of %s\n", id, bytes, temp_buf);
        usleep(150000);
    }
    return NULL;
}

void* consumer(void* arg) {
    int id = *(int*)arg;
    char recv_buf[81];

    while (1) {
        int bytes = mymsgget(&queue, recv_buf, sizeof(recv_buf));
        if (bytes == 0) {
            printf("[Consumer %d] shutting down\n", id);
            break;
        }

        printf("     -> [Consumer %d] Received %d bytes: %s\n", id, bytes, recv_buf);
        usleep(250000);
    }
    return NULL;
}

int main() {
    mymsginit(&queue);

    pthread_t prod[2], cons[2];
    int prod_ids[2] = {1, 2};
    int cons_ids[2] = {1, 2};
    int code = 0;

    for (int i = 0; i < 2; i++) {
        code = pthread_create(&cons[i], NULL, consumer, &cons_ids[i]);
        if (code != 0) {
            fprintf(stderr, "create consumer Num%d: %s\n", i+1, strerror(code));
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < 2; i++) {
        code = pthread_create(&prod[i], NULL, producer, &prod_ids[i]);
        if (code != 0) {
            fprintf(stderr, "create producer Num%d: %s\n", i+1, strerror(code));
            exit(EXIT_FAILURE);
        }
    }

    sleep(3);

    mymsgdrop(&queue);

    for (int i = 0; i < 2; i++) {
        code = pthread_join(prod[i], NULL);
        if (code != 0) {
            fprintf(stderr, "join producer Num%d: %s\n", i+1, strerror(code));
            exit(EXIT_FAILURE);
        }
        code = pthread_join(cons[i], NULL);
        if (code != 0) {
            fprintf(stderr, "join consumer Num%d: %s\n", i+1, strerror(code));
            exit(EXIT_FAILURE);
        }
    }

    mymsgdestroy(&queue);

    return 0;
}
