#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER 80
#define SORT_DELAY 5

void *bubble_sort(void *arg) {
    List *list = (List *)arg;

    while (1) {
        int swapped = 1;
        while (swapped) {
            Node *prev;
            Node *curr;
            Node *next;
            swapped = 0;
            prev = list->head;
            pthread_rwlock_wrlock(&prev->lock);
            curr = prev->next;

            if (!curr) {
                pthread_rwlock_unlock(&prev->lock);
                break;
            }

            pthread_rwlock_wrlock(&curr->lock);

            next = curr->next;

            while (next) {
                pthread_rwlock_wrlock(&next->lock);
                if (strcmp(curr->value, next->value) > 0) {
                    curr->next = next->next;
                    prev->next = next;
                    next->next = curr;
                    swapped = 1;

                    pthread_rwlock_unlock(&prev->lock);

                    prev = next;
                    next = curr->next;
                } else {
                    pthread_rwlock_unlock(&prev->lock);

                    prev = curr;
                    curr = next;
                    next = curr->next;
                }
            }

            pthread_rwlock_unlock(&prev->lock);
            pthread_rwlock_unlock(&curr->lock);
        }
        sleep(SORT_DELAY);
    }
}

int main() {
    List my_list;
    char buffer[MAX_BUFFER + 1];
    int big = 0;
    pthread_t thread_id;

    init_list(&my_list);
    int rc = pthread_create(&thread_id, NULL, bubble_sort, &my_list);
    if (rc != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc));
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

    return EXIT_SUCCESS;
}
