#include "list.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER 80
#define THREAD_COUNT 1

int main() {
    List *my_list;
    char buffer[MAX_BUFFER + 1];
    int big = 0;
    pthread_t thread_id[THREAD_COUNT];

    my_list = init_list();
    for (int i = 0; i < THREAD_COUNT; i++){
        int rc = pthread_create(&thread_id[i], NULL, bubble_sort, my_list);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }

    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (buffer[0] == '\n') {
            if (!big) {
                print_list(my_list);
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

        push_front(my_list, buffer);
    }

    return EXIT_SUCCESS;
}
