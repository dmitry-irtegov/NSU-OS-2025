#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LEN 80

volatile int allow_sort = 0;
volatile int working = 1;
typedef struct node {
    char *data;
    struct node *next;
    pthread_mutex_t mutex;
} node_t;

typedef struct {
    node_t *head;
    pthread_mutex_t head_mutex;
} list_t;

list_t list;

void list_init(list_t *list) {
    list->head = NULL;
    int err = pthread_mutex_init(&list->head_mutex, NULL);
    if (err != 0) {
        fprintf(stderr, "list init error: %s\n",strerror(err));
        exit(1);
    }
}

void list_push(list_t *list, const char *str) {
    node_t *node = malloc(sizeof(node_t));
    if (!node) {
        perror("failure malloc node");
        exit(1);
    }
    node->data = strdup(str);
    if (!node->data) {
        perror("strdup failrue");
        free(node);
        exit(1);
    }
    node->next = NULL;
    int err = pthread_mutex_init(&node->mutex, NULL);
    if (err != 0) {
        fprintf(stderr, "mutex init error: %s\n", strerror(err));
        exit(1);
    }

    pthread_mutex_lock(&list->head_mutex);
    node->next = list->head;
    list->head = node;
    pthread_mutex_unlock(&list->head_mutex);
}

void list_print(list_t *list) {
    pthread_mutex_lock(&list->head_mutex);
    node_t *cur = list->head;
    printf("**********************\n");
    if (!cur) {
        printf("List is empty\n");
    }
    while (cur) {
        pthread_mutex_lock(&cur->mutex);
        printf("%s\n", cur->data);
        pthread_mutex_unlock(&cur->mutex);
        cur = cur->next;
    }
    printf("**********************\n");
    pthread_mutex_unlock(&list->head_mutex);
}

static void swap_nodes(list_t *list, node_t *prev, node_t *curr, node_t *next) {
    pthread_mutex_lock(&list->head_mutex);
    if (prev) prev->next = next;
    curr->next = next->next;
    next->next = curr;
    if (!prev) list->head = next;
    pthread_mutex_unlock(&list->head_mutex);
}

int bubble_sort_step(list_t *list) {
    int swapped = 0;
    node_t *prev = NULL;
    node_t *curr;

    pthread_mutex_lock(&list->head_mutex);
    curr = list->head;
    pthread_mutex_unlock(&list->head_mutex);

    while (curr) {
        node_t *next = curr->next;

        if (prev) pthread_mutex_lock(&prev->mutex);
        pthread_mutex_lock(&curr->mutex);

        if (!next) {
            pthread_mutex_unlock(&curr->mutex);
            if (prev) pthread_mutex_unlock(&prev->mutex);
            break;
        }
        pthread_mutex_lock(&next->mutex);

        if (curr->data && next->data && strcmp(curr->data, next->data) > 0) {
            swap_nodes(list, prev, curr, next);
            swapped = 1;
            if (prev) pthread_mutex_unlock(&prev->mutex);
            pthread_mutex_unlock(&curr->mutex);
            pthread_mutex_unlock(&next->mutex);

            printf("Sort step:\n");
            list_print(list);
            usleep(1000000);

            prev = next;
        } else {
            if (prev) pthread_mutex_unlock(&prev->mutex);
            pthread_mutex_unlock(&curr->mutex);
            pthread_mutex_unlock(&next->mutex);

            prev = curr;
            curr = next;
        }
    }
    return swapped;
}

void list_destroy(list_t *list) {
    pthread_mutex_lock(&list->head_mutex);
    node_t *curr = list->head;
    list->head = NULL;
    pthread_mutex_unlock(&list->head_mutex);

    while (curr) {
        node_t *next = curr->next;
        int err = pthread_mutex_destroy(&curr->mutex);
        if (err != 0) {
            fprintf(stderr, "destroy mutex error: %s\n", strerror(err));
        }
        free(curr->data);
        free(curr);
        curr = next;
    }
    int err = pthread_mutex_destroy(&list->head_mutex);
    if (err != 0) {
        fprintf(stderr, "destroy mutex error: %s\n",strerror(err));
    }
}

void* sorter_thread(void* arg) {
    list_t *list = (list_t*)arg;
    while (working) {
        if(!allow_sort) {
            usleep(5000000);
            continue;
        }
        int swapped = bubble_sort_step(list);
        if (!swapped){
            usleep(1000000);
        }
        else{
            usleep(200000);
        }

    }
    return NULL;
}
int main() {
    list_init(&list);
    pthread_t thread;
    int err = pthread_create(&thread, NULL, sorter_thread, &list);
    if (err != 0) {
        fprintf(stderr, "pthread_create error: %s\n", strerror(err));
        exit(1);
    }
    char buffer[MAX_LEN + 1];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (buffer[0] == '\n') {
            printf("Current state of list:\n");
            list_print(&list);
            allow_sort = 1;
            continue;
        }
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';
        len = strlen(buffer);
        for (size_t i = 0; i < len; i += MAX_LEN) {
            char chunk[MAX_LEN+1];
            size_t chunk_len = (len - i > MAX_LEN) ? MAX_LEN : (len - i);
            memcpy(chunk, buffer + i, chunk_len);
            chunk[chunk_len] = 0;
            list_push(&list, chunk);
        }
    }
    working = 0;
    err = pthread_join(thread, NULL);
    if (err != 0){
        fprintf(stderr, "pthread_join error: %s\n", strerror(err));
        exit(1);
    }
    list_destroy(&list);
    return 0;
}
