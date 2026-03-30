#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <semaphore.h>

#define MAX_MSG_LEN 80
#define QUEUE_CAPACITY 10

typedef struct {
    char buffer[QUEUE_CAPACITY][MAX_MSG_LEN + 1];
    int head;
    int tail;
    volatile int dropped;
    sem_t empty_slots;
    sem_t filled_slots;
    sem_t mutex;
} Queue;

void mymsginit(Queue *q);
int mymsgput(Queue *q, const char *msg);
int mymsgget(Queue *q, char *buf, size_t bufSize);
void mymsgdrop(Queue *q);
void mymsgdestroy(Queue *q);

#endif
