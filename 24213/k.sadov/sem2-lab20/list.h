#ifndef LIST_H
#define LIST_H

#include <pthread.h>

typedef struct Node {
    char *value;
    struct Node *next;
    pthread_rwlock_t lock;
} Node;

typedef struct {
    Node *head;
    pthread_rwlock_t h_lock;
} List;

void init_list(List *list);
void push_front(List *list, const char *str);
void print_list(List *list);
void clear_list(List *list);

#endif
