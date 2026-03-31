#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

void init_list(List *list) {
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
