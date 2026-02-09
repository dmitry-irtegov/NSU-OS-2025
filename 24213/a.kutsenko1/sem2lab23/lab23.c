#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LINES 100
#define SLEEP_MULTIPLIER 10000

typedef struct Node {
    char *str;
    struct Node *next;
} Node;

Node *sorted_list = NULL;
pthread_mutex_t mutex_list = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char *str;
    int length;
} ThreadData;

void insert_into_sorted_list(char *str) {
    Node *new_n = (Node *)malloc(sizeof(Node));
    if (!new_n) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    new_n->str = str;
    new_n->next = NULL;
    pthread_mutex_lock(&mutex_list);
    new_n->next = sorted_list;
    sorted_list = new_n;
    pthread_mutex_unlock(&mutex_list);
}

void print_list_reverse(Node *node, int *counter) {
    if (node == NULL) {
        return;
    }
    print_list_reverse(node->next, counter);
    printf("%d: %s (length: %zu)\n", (*counter)++, node->str, strlen(node->str));
}

void print_entire_list() {
    int counter = 1;
    print_list_reverse(sorted_list, &counter);
    if (counter == 1) {
        printf("(no strings in list)\n");
    }
}

void *sort_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    usleep(data->length * SLEEP_MULTIPLIER);
    insert_into_sorted_list(data->str);
    free(data);
    return NULL;
}

int main() {
    char *lines[MAX_LINES];
    char buffer[BUFSIZ];
    int line_count = 0;
    pthread_t threads[MAX_LINES];
    while (line_count < MAX_LINES && fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        lines[line_count] = strdup(buffer);
        if (!lines[line_count]) {
            perror("strdup failed");
            pthread_mutex_destroy(&mutex_list);
            exit(EXIT_FAILURE);
        }
        line_count++;
    }
    if (line_count == 0) {
        return 0;
    }
    for (int i = 0; i < line_count; i++) {
        ThreadData *data = (ThreadData *)malloc(sizeof(ThreadData));
        if (!data) {
            perror("malloc failed");
            for (int j = 0; j < line_count; j++) {
                free(lines[j]);
            }
            pthread_mutex_destroy(&mutex_list);
            exit(EXIT_FAILURE);
        }
        data->str = lines[i];
        data->length = strlen(lines[i]);
        int err = pthread_create(&threads[i], NULL, sort_thread, data);
        if (err != 0) {
            fprintf(stderr, "pthread_create failed: %s\n", strerror(err));
            for (int j = 0; j < line_count; j++) {
                free(lines[j]);
            }
            free(data);
            pthread_mutex_destroy(&mutex_list);
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < line_count; i++) {
        int err = pthread_join(threads[i], NULL);
        if (err != 0) {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(err));
            for (int j = 0; j < line_count; j++) {
                free(lines[j]);
            }
            pthread_mutex_destroy(&mutex_list);
            exit(EXIT_FAILURE);
        }
    }
    print_entire_list();
    Node *current = sorted_list;
    while (current != NULL) {
        Node *temp = current;
        current = current->next;
        free(temp->str);
        free(temp);
    }
    pthread_mutex_destroy(&mutex_list);
    return 0;
}
