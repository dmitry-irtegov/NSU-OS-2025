#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER 80
#define SORT_DELAY 5

volatile int is_running = 1;

void* bubble_sort(void *arg) {
    List *list = (List *)arg;

    while (is_running) {
        int swapped = 1;

        while (swapped) {
            swapped = 0;
            pthread_rwlock_wrlock(&list->h_lock);

            Node *prev = NULL;
            Node *curr = list->head;

            if (!curr || !curr->next) {
                pthread_rwlock_unlock(&list->h_lock);
                break;
            }

            pthread_rwlock_wrlock(&curr->lock);
            Node *next = curr->next;
            pthread_rwlock_wrlock(&next->lock);

            while (next) {
                if (strcmp(curr->value, next->value) > 0) {
                    prev ? (prev->next = next) : (list->head = next);

                    curr->next = next->next;
                    next->next = curr;
                    swapped = 1;

                    pthread_rwlock_unlock(prev ? &prev->lock : &list->h_lock);

                    prev = next;
                    next = curr->next;
                } else {
                    pthread_rwlock_unlock(prev ? &prev->lock : &list->h_lock);

                    prev = curr;
                    curr = next;
                    next = curr->next;
                }

                if (next) {
                    pthread_rwlock_wrlock(&next->lock);
                }
            }

            if (prev) {
                pthread_rwlock_unlock(&prev->lock);
            }
            if (curr) {
                pthread_rwlock_unlock(&curr->lock);
            }
        }
        sleep(SORT_DELAY);
    }
    return NULL;
}

int main() {
    List my_list;
    init_list(&my_list);
    char buffer[MAX_BUFFER + 1];
    int big = 0;
    pthread_t thread_id;

    int rc = pthread_create(&thread_id, NULL, bubble_sort, &my_list);
    if (rc != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc));
        clear_list(&my_list);
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (buffer[0] == '\n') {
            if (!big) {
                print_list(&my_list);
            }
            big = 0;
            continue;
        }

        size_t len = strlen(buffer);
        if (buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            big = 0;
        } else {
            big = 1;
        }

        push_front(&my_list, buffer);
    }

    is_running = 0;
    rc = pthread_join(thread_id, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }
    clear_list(&my_list);
    return EXIT_SUCCESS;
}
