#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

void mymsginit(Queue *q) {
    q->head = 0;
    q->tail = 0;
    q->dropped = 0;

    if (sem_init(&q->empty_slots, 0, QUEUE_CAPACITY) != 0) {
        perror("sem_init empty_slots");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&q->filled_slots, 0, 0) != 0) {
        perror("sem_init filled_slots");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&q->mutex, 0, 1) != 0) {
        perror("sem_init mutex");
        exit(EXIT_FAILURE);
    }
}

int mymsgput(Queue *q, const char *msg) {
    if (sem_wait(&q->empty_slots) != 0) {
        perror("sem_wait empty_slots in put");
        exit(EXIT_FAILURE);
    }
    if (sem_wait(&q->mutex) != 0) {
        perror("sem_wait mutex in put");
        exit(EXIT_FAILURE);
    }

    if (q->dropped) {
        if (sem_post(&q->mutex) != 0) {
            perror("sem_post mutex in put");
            exit(EXIT_FAILURE);
        }
        if (sem_post(&q->empty_slots) != 0) {
            perror("sem_post empty_slots in put");
            exit(EXIT_FAILURE);
        }
        return 0;
    }

    size_t len = strlen(msg);
    if (len > MAX_MSG_LEN) {
        len = MAX_MSG_LEN;
    }

    strncpy(q->buffer[q->tail], msg, len);
    q->buffer[q->tail][len] = '\0';
    q->tail = (q->tail + 1) % QUEUE_CAPACITY;

    if (sem_post(&q->mutex) != 0) {
        perror("sem_post mutex in put");
        exit(EXIT_FAILURE);
    }
    if (sem_post(&q->filled_slots) != 0) {
        perror("sem_post filled_slots in put");
        exit(EXIT_FAILURE);
    }

    return (int)len;
}

int mymsgget(Queue *q, char *buf, size_t bufSize) {
    if (sem_wait(&q->filled_slots) != 0) {
        perror("sem_wait filled_slots in get");
        exit(EXIT_FAILURE);
    }
    if (sem_wait(&q->mutex) != 0) {
        perror("sem_wait mutex in get");
        exit(EXIT_FAILURE);
    }

    if (q->dropped) {
        if (sem_post(&q->mutex) != 0) {
            perror("sem_post mutex in get");
            exit(EXIT_FAILURE);
        }
        if (sem_post(&q->filled_slots) != 0) {
            perror("sem_post filled_slots in get");
            exit(EXIT_FAILURE);
        }
        return 0;
    }

    size_t copy_len = strlen(q->buffer[q->head]);

    if (bufSize > 0) {
        if (copy_len >= bufSize) {
            copy_len = bufSize - 1;
        }
        strncpy(buf, q->buffer[q->head], copy_len);
        buf[copy_len] = '\0';
    } else {
        copy_len = 0;
    }

    q->head = (q->head + 1) % QUEUE_CAPACITY;

    if (sem_post(&q->mutex) != 0) {
        perror("sem_post mutex in get");
        exit(EXIT_FAILURE);
    }
    if (sem_post(&q->empty_slots) != 0) {
        perror("sem_post empty_slots in get");
        exit(EXIT_FAILURE);
    }

    return (int)copy_len;
}

void mymsgdrop(Queue *q) {
    if (sem_wait(&q->mutex) != 0) {
        perror("sem_wait mutex in drop");
        exit(EXIT_FAILURE);
    }
    q->dropped = 1;
    if (sem_post(&q->mutex) != 0) {
        perror("sem_post mutex in drop");
        exit(EXIT_FAILURE);
    }

    if (sem_post(&q->empty_slots) != 0) {
        perror("sem_post empty_slots wakeup");
        exit(EXIT_FAILURE);
    }
    if (sem_post(&q->filled_slots) != 0) {
        perror("sem_post filled_slots wakeup");
        exit(EXIT_FAILURE);
    }
}

void mymsgdestroy(Queue *q) {
    if (sem_destroy(&q->empty_slots) != 0) {
        perror("sem_destroy empty_slots");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&q->filled_slots) != 0) {
        perror("sem_destroy filled_slots");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&q->mutex) != 0) {
        perror("sem_destroy mutex");
        exit(EXIT_FAILURE);
    }
}
