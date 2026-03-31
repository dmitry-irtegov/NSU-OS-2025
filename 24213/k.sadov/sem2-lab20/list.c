#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

void init_list(List *list) {
    list->head = NULL;
    int rc = pthread_rwlock_init(&list->h_lock, NULL);
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
        free(new_node);
        exit(EXIT_FAILURE);
    }

    int rc = pthread_rwlock_init(&new_node->lock, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_rwlock_init: push_to_list %s\n", strerror(rc));
        free(new_node->value);
        free(new_node);
        exit(EXIT_FAILURE);
    }

    pthread_rwlock_wrlock(&list->h_lock);
    new_node->next = list->head;
    list->head = new_node;
    pthread_rwlock_unlock(&list->h_lock);
}

void print_list(List *list) {
    pthread_rwlock_rdlock(&list->h_lock);
    Node *current = list->head;

    if (current) {
        pthread_rwlock_rdlock(&current->lock);
    }
    pthread_rwlock_unlock(&list->h_lock);
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

void clear_list(List *list) {
    pthread_rwlock_wrlock(&list->h_lock);
    Node *curr = list->head;
    list->head = NULL;
    pthread_rwlock_unlock(&list->h_lock);

    while (curr) {
        Node *tmp = curr;
        curr = curr->next;

        pthread_rwlock_destroy(&tmp->lock);
        free(tmp->value);
        free(tmp);
    }
    pthread_rwlock_destroy(&list->h_lock);
}
