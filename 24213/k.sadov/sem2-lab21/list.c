#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "list.h"

#define SORT_DELAY 5
#define STEP_DELAY 1

#define wlock(rwlock) pthread_rwlock_wrlock(rwlock); {
#define unlock(rwlock) } pthread_rwlock_unlock(rwlock);
#define rlock(rwlock) pthread_rwlock_rdlock(rwlock); {

typedef struct Node {
    char *value;
    struct Node *next;
    pthread_rwlock_t lock;
} Node;

struct List {
    Node *head;
};

List *init_list() {
    List *list = malloc(sizeof(*list));
    if (!list) {
        perror("malloc list");
        exit(EXIT_FAILURE);
    }

    list->head = malloc(sizeof(Node));
    if (!list->head) {
        perror("malloc head");
        exit(EXIT_FAILURE);
    }
    list->head->value = NULL;
    list->head->next = NULL;
    int rc = pthread_rwlock_init(&list->head->lock, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_rwlock_init: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }
    return list;
}

void push_front(List *list, const char *str) {
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) {
        perror("malloc new_node");
        exit(EXIT_FAILURE);
    }

    new_node->value = strdup(str);
    if (!new_node->value) {
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }

    int rc = pthread_rwlock_init(&new_node->lock, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_rwlock_init: push_front %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }

    pthread_rwlock_wrlock(&list->head->lock);
    new_node->next = list->head->next;
    list->head->next = new_node;
    pthread_rwlock_unlock(&list->head->lock);
}

void print_list(List *list) {
    pthread_rwlock_rdlock(&list->head->lock);
    Node *current = list->head->next;

    if (current) {
        pthread_rwlock_rdlock(&current->lock);
    }
    pthread_rwlock_unlock(&list->head->lock);
    printf("--------- LIST START ---------\n");
    while (current) {
        printf("%s\n", current->value);

        Node *next = current->next;
        if (next) {
            pthread_rwlock_rdlock(&next->lock);
        }

        pthread_rwlock_unlock(&current->lock);
        current = next;
    }
    printf("--------- LIST END ---------\n");
}

static int sort_one_step(Node **start, int *swapped, int *end) {
    Node *prev;
    Node *curr;
    Node *next;
    int did_swap;

    prev = *start;
    curr = NULL;
    next = NULL;
    did_swap = 0;
    *end = 0;

    wlock(&prev->lock)
        curr = prev->next;
        if (!curr) {
            *end = 1;
        } else {
            wlock(&curr->lock)
                next = curr->next;
                if (!next) {
                    *end = 1;
                } else {
                    wlock(&next->lock)
                        if (strcmp(curr->value, next->value) > 0) {
                            Node *after = next->next;

                            curr->next = after;
                            prev->next = next;
                            next->next = curr;

                            *swapped = 1;
                            *start = next;
                            did_swap = 1;
                        } else {
                            *start = curr;
                        }
                    unlock(&next->lock)
                }
            unlock(&curr->lock)
        }
    unlock(&prev->lock)
    return did_swap;
}

void *bubble_sort(void *arg) {
    List *list = (List *)arg;

    while (1) {
        int swapped;
        swapped = 1;
        while (swapped) {
            Node *start;
            int end;

            start = list->head;
            end = 0;
            swapped = 0;

            while (!end) {
                int did_swap;

                did_swap = sort_one_step(&start, &swapped, &end);

                if (did_swap) {
                    sleep(STEP_DELAY);
                }
            }

        }
        sleep(SORT_DELAY);
    }
}
