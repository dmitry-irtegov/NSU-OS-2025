#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "stack.h"

void stack_init(stack* st, pthread_mutex_t* mutex) {
    st->top = NULL;
    st->mutex = mutex;
}

void stack_push(stack* st, char* buffer, int len) {
    pthread_mutex_lock(st->mutex);
    stack_el* new = (stack_el*)malloc(sizeof(stack_el));
    if (!new) {
        perror("malloc");
        exit(1);
    }
    
    char* str = (char*)malloc(len);
    if (!str) {
        perror("malloc");
        exit(1);
    }

    strncpy(str, buffer, len);

    new->next = st->top;
    new->str = str;
    st->top = new;
    pthread_mutex_unlock(st->mutex);
}

void stack_print(stack *st) {
    pthread_mutex_lock(st->mutex);
    stack_el* cur = st->top;
    while (cur != NULL) {
        printf("%s", cur->str);
        if (cur->next != NULL) {
            printf(" -> ");
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(st->mutex);
}

void stack_sort(stack* st) {
    pthread_mutex_lock(st->mutex);
    int sorted = 0;
    while (!sorted) {
        sorted = 1;

        stack_el* cur = st->top;
        while (cur != NULL) {
            if (cur->next == NULL) {
                break;
            }

            if (strcmp(cur->str, cur->next->str) > 0) {
                sorted = 0;
                char* tmp = cur->str;
                cur->str = cur->next->str;
                cur->next->str = tmp;
            }

            cur = cur->next;
        }
    }
    pthread_mutex_unlock(st->mutex);
}
