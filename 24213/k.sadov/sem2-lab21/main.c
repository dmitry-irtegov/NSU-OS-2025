#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER 80
#define SORT_DELAY 5
#define STEP_DELAY 1

int position_locker(List *list, Node *pos, Node **prev_out, Node **curr_out) {
    Node *prev;
    Node *curr;
    Node *next;

    prev = list->head;
    pthread_rwlock_wrlock(&prev->lock);

    curr = prev->next;
    if (!curr) {
        pthread_rwlock_unlock(&prev->lock);
        return 0;
    }

    pthread_rwlock_wrlock(&curr->lock);

    while (pos && curr != pos) {
        next = curr->next;

        if (!next) {
            *prev_out = prev;
            *curr_out = curr;
            return 1;
        }

        pthread_rwlock_wrlock(&next->lock);
        pthread_rwlock_unlock(&prev->lock);

        prev = curr;
        curr = next;
    }

    *prev_out = prev;
    *curr_out = curr;
    return 1;
}

void *bubble_sort(void *arg) {
    List *list = (List *)arg;

    while (1) {
        int swapped = 1;
        while (swapped) {
            Node *pos = NULL;
            int end = 0;
            swapped = 0;
            while (!end) {
                Node *prev;
                Node *curr;
                Node *next;
                int slept = 0;

                if (!position_locker(list, pos, &prev, &curr)) {
                    break;
                }

                next = curr->next;

                while (next && !slept) {
                    pthread_rwlock_wrlock(&next->lock);

                    if (strcmp(curr->value, next->value) > 0) {
                        Node *after = next->next;

                        curr->next = after;
                        prev->next = next;
                        next->next = curr;

                        swapped = 1;
                        pos = curr;

                        pthread_rwlock_unlock(&prev->lock);
                        pthread_rwlock_unlock(&curr->lock);
                        pthread_rwlock_unlock(&next->lock);

                        sleep(STEP_DELAY);
                        slept = 1;
                    } else {
                        pthread_rwlock_unlock(&prev->lock);

                        prev = curr;
                        curr = next;
                        next = curr->next;
                        pos = curr;
                    }
                }

                if (!slept) {
                    pthread_rwlock_unlock(&prev->lock);
                    pthread_rwlock_unlock(&curr->lock);
                    end = 1;
                }
            }

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
