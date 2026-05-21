#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "sort.h"

static void swap_nodes(singly_linked_list_t *list, node_t *prev,
                       node_t *curr, node_t *next) {
    if (prev == NULL) {
    } else {
        prev->next = next;
    }
    curr->next = next->next;
    next->next = curr;

    pthread_mutex_lock(&list->mutex);
    if (prev == NULL) {
        list->head = next;
    }
    if (list->tail == curr) {
        list->tail = next;
    } else if (list->tail == next) {
        list->tail = curr;
    }
    pthread_mutex_unlock(&list->mutex);
}

int bubble_sort_step(singly_linked_list_t *list) {
    int swapped = 0;
    node_t *prev = NULL;
    node_t *curr = NULL;

    pthread_mutex_lock(&list->mutex);
    curr = list->head;
    pthread_mutex_unlock(&list->mutex);

    while (curr) {
        node_t *next = NULL;

        if (prev) {
            pthread_mutex_lock(&prev->mutex);
        }
        pthread_mutex_lock(&curr->mutex);

        next = curr->next;
        if (!next) {
            pthread_mutex_unlock(&curr->mutex);
            if (prev) {
                pthread_mutex_unlock(&prev->mutex);
            }
            break;
        }

        pthread_mutex_lock(&next->mutex);

        if (curr->data && next->data && strcmp(curr->data, next->data) > 0) {
            swap_nodes(list, prev, curr, next);
            swapped = 1;

            if (prev) {
                pthread_mutex_unlock(&prev->mutex);
            }
            pthread_mutex_unlock(&curr->mutex);
            pthread_mutex_unlock(&next->mutex);
            sleep(1);

            prev = next;
        } else {
            if (prev) {
                pthread_mutex_unlock(&prev->mutex);
            }
            pthread_mutex_unlock(&curr->mutex);
            pthread_mutex_unlock(&next->mutex);

            prev = curr;
            curr = next;
        }
    }
    return swapped;
}

void bubble_sort(singly_linked_list_t *list) {
    while (bubble_sort_step(list)) {
    }
}
