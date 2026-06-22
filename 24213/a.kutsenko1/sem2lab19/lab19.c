#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LEN 80


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

typedef struct {
    list_t *list;
    node_t *prev; 
    node_t *curr;
    node_t *next;
    int head_locked; 
} cursor_t;

void cursor_init(cursor_t *c, list_t *list) {
    c->list = list;
    c->prev = NULL;
    c->curr = list->head;
    c->next = c->curr ? c->curr->next : NULL;
    c->head_locked = 1;
}

void cursor_advance(cursor_t *c) {
    if (c->prev) {
        pthread_mutex_unlock(&c->prev->mutex);
    } else if (c->head_locked) {
        pthread_mutex_unlock(&c->list->head_mutex);
        c->head_locked = 0;
    }

    c->prev = c->curr;
    c->curr = c->next;
    c->next = c->curr ? c->curr->next : NULL;

    if (c->next) {
        pthread_mutex_lock(&c->next->mutex);
    }
}

void cursor_swap(cursor_t *c) {
    c->curr->next = c->next->next;
    c->next->next = c->curr;

    if (c->prev) {
        c->prev->next = c->next;
    } else {
        c->list->head = c->next;
    }

    node_t *tmp = c->curr;
    c->curr = c->next;
    c->next = tmp;
}

void cursor_unlock_all(cursor_t *c) {
    if (c->prev) {
        pthread_mutex_unlock(&c->prev->mutex);
    } else if (c->head_locked) {
        pthread_mutex_unlock(&c->list->head_mutex);
        c->head_locked = 0;
    }
    if (c->curr) {
        pthread_mutex_unlock(&c->curr->mutex);
    }
    if (c->next) {
        pthread_mutex_unlock(&c->next->mutex);
    }
}

list_t list;

void list_init(list_t *list) {
    list->head = NULL;
    int err = pthread_mutex_init(&list->head_mutex, NULL);
    if (err != 0) {
        fprintf(stderr, "list init error: %s\n", strerror(err));
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
        perror("strdup failure");
        free(node);
        exit(1);
    }
    node->next = NULL;
    int err = pthread_mutex_init(&node->mutex, NULL);
    if (err != 0) {
        fprintf(stderr, "mutex init error: %s\n", strerror(err));
        free(node->data);
        free(node);
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

int bubble_sort_step(list_t *list) {
    pthread_mutex_lock(&list->head_mutex);

    if (!list->head || !list->head->next) {
        pthread_mutex_unlock(&list->head_mutex);
        return 0;
    }

    cursor_t c;
    cursor_init(&c, list);
    
    pthread_mutex_lock(&c.curr->mutex);
    pthread_mutex_lock(&c.next->mutex);

    while (c.next) {
        if (c.curr->data && c.next->data && strcmp(c.curr->data, c.next->data) > 0) {
            cursor_swap(&c);
            cursor_unlock_all(&c);
            return 1;
        }
        cursor_advance(&c);
    }

    cursor_unlock_all(&c);
    return 0;
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
        fprintf(stderr, "destroy mutex error: %s\n", strerror(err));
    }
}


void* sorter_thread(void* arg) {
    list_t *l = (list_t*)arg;
    
    while (working) {
        usleep(5000000);
        if (!working) {
            break;
        }

        printf("\n5 seconds passed. Starting sorting process...\n");

        int swapped = 1;
        while (swapped && working) {
            swapped = bubble_sort_step(l);
            if (swapped) {
                printf("Step of sorting:\n");
                list_print(l);
                usleep(1000000); 
            }
        }
        
        printf("Sorting completed. Waiting for next 5 seconds...\n\n");
        
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
            printf("\nCurrent state of list:\n");
            list_print(&list);
            continue; 
        }

        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        len = strlen(buffer);
        for (size_t i = 0; i < len; i += MAX_LEN) {
            char chunk[MAX_LEN + 1];
            size_t chunk_len = (len - i > MAX_LEN) ? MAX_LEN : (len - i);
            memcpy(chunk, buffer + i, chunk_len);
            chunk[chunk_len] = '\0';
            list_push(&list, chunk);
        }
    }

    working = 0;
    err = pthread_join(thread, NULL);
    if (err != 0) {
        fprintf(stderr, "pthread_join error: %s\n", strerror(err));
        exit(1);
    }
    
    list_destroy(&list);
    return 0;
}
