#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define MAX_LEN 80

volatile int keep_running_flag = 1;

typedef struct Node {
    char *data;
    pthread_mutex_t mutex;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    pthread_mutex_t head_mutex;
} List;

List list;

void list_init(List *l) {
    l->head = NULL;
    int code = pthread_mutex_init(&l->head_mutex, NULL);
    if (code != 0) {
        fprintf(stderr, "init head mutex error: %s\n", strerror(code));
        exit(EXIT_FAILURE);
    }
}

void list_push(List *l, const char *str) {
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) {
        perror("malloc node");
        exit(EXIT_FAILURE);
    }

    new_node->data = malloc(strlen(str) + 1);
    if (!new_node->data) {
        perror("malloc data");
        free(new_node);
        exit(EXIT_FAILURE);
    }
    strcpy(new_node->data, str);

    int code = pthread_mutex_init(&new_node->mutex, NULL);
    if (code != 0) {
        fprintf(stderr, "init node mutex error: %s\n", strerror(code));
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&l->head_mutex);
    new_node->next = l->head;
    l->head = new_node;
    pthread_mutex_unlock(&l->head_mutex);
}

void list_print(List *l) {
    pthread_mutex_lock(&l->head_mutex);
    Node *curr = l->head;
    if (curr) {
        pthread_mutex_lock(&curr->mutex);
    }
    pthread_mutex_unlock(&l->head_mutex);

    printf("----------------------\n");
    while (curr) {
        printf("%s\n", curr->data);
        
        Node *next = curr->next;
        if (next) {
            pthread_mutex_lock(&next->mutex);
        }
        pthread_mutex_unlock(&curr->mutex);
        curr = next;
    }
    printf("----------------------\n");
}

void* sorter_thread(void* arg) {
    List *l = (List*)arg;
    while (keep_running_flag) {
        int swapped;
        do {
            swapped = 0;
            pthread_mutex_lock(&l->head_mutex);
            
            Node *prev = NULL;
            Node *curr = l->head;

            if (!curr || !curr->next) {
                pthread_mutex_unlock(&l->head_mutex);
                break;
            }

            pthread_mutex_lock(&curr->mutex);
            Node *next = curr->next;
            pthread_mutex_lock(&next->mutex);

            while (next) {
                if (strcmp(curr->data, next->data) > 0) {
                    if (prev == NULL) {
                        l->head = next;
                    } else {
                        prev->next = next;
                    }
                    curr->next = next->next;
                    next->next = curr;
                    swapped = 1;

                    if (prev == NULL) { pthread_mutex_unlock(&l->head_mutex); }
                    else { pthread_mutex_unlock(&prev->mutex); }

                    prev = next;
                    next = curr->next;
                } else {
                    if (prev == NULL) { pthread_mutex_unlock(&l->head_mutex); }
                    else { pthread_mutex_unlock(&prev->mutex); }

                    prev = curr;
                    curr = next;
                    next = curr->next;
                }

                if (next) {
                    pthread_mutex_lock(&next->mutex);
                }
            }

            pthread_mutex_unlock(&prev->mutex);
            pthread_mutex_unlock(&curr->mutex);

        } while (swapped);
        
        sleep(5);
    }
    return NULL;
}

void list_destroy(List *l) {
    pthread_mutex_lock(&l->head_mutex);
    Node *curr = l->head;
    l->head = NULL;
    pthread_mutex_unlock(&l->head_mutex);

    while (curr) {
        Node *next = curr->next;

        int code = pthread_mutex_destroy(&curr->mutex);
        if (code != 0) {
            fprintf(stderr, "destroy node mutex error: %s\n", strerror(code));
        }
        
        free(curr->data);
        free(curr);
        curr = next;
    }

    int code = pthread_mutex_destroy(&l->head_mutex);
    if (code != 0) {
        fprintf(stderr, "destroy head mutex error: %s\n", strerror(code));
    }
}

int main() {
    list_init(&list);
    
    pthread_t tid;
    int code = pthread_create(&tid, NULL, sorter_thread, &list);
    if (code != 0) {
        fprintf(stderr, "pthread create error: %s\n", strerror(code));
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_LEN + 1];
    int long_line_flag = 0;

    while (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);

        if (buffer[0] == '\n' && !long_line_flag) {
            list_print(&list);
            continue;
        }

        if (buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            list_push(&list, buffer);
            long_line_flag = 0;
        } else {
            list_push(&list, buffer);
            long_line_flag = 1;
        }
    }

    keep_running_flag = 0;
    code = pthread_join(tid, NULL);
    if (code != 0) {
        fprintf(stderr, "pthread join error: %s\n", strerror(code));
        exit(EXIT_FAILURE);   
    }
    list_destroy(&list);

    exit(EXIT_SUCCESS);
}
