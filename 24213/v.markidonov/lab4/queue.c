#include <stdlib.h>
#include "queue.h"

void queue_init(queue* q) {
    q->first = NULL;
    q->last = NULL;
}

int enqueue(queue* q, char* str) {
    qelem *new;
    if (!(new = (qelem*)malloc(sizeof(qelem)))) {
        return -1;
    }

    new->next = NULL;
    new->str = str;

    if (q->first == NULL) {
        q->first = new;
        q->last = new;
        return 0;
    }

    q->last->next = new;
    q->last = new;
    return 0;
}

char* dequeue(queue* q) {
    qelem *dequeue_elem = q->first;

    if (dequeue_elem == NULL) {
        return NULL;
    }

    q->first = dequeue_elem->next;
    if (q->first == NULL) {
        q->last = NULL;
    }

    char *str = dequeue_elem->str;
    free(dequeue_elem);
    return str;
}

void queue_free(queue* q) {
    qelem *cur = q->first;
    while (cur != NULL) {
        qelem* next = cur->next;
        free(cur);
        cur = next;
    }
}

