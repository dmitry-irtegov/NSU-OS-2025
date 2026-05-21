#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct Node {
    char data[81];
    struct Node *next;
} Node;

Node *head = NULL;
pthread_mutex_t mutex;

void push_front(const char *str) {
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) {
        perror("malloc");
        exit(1);
    }

    strcpy(new_node->data, str);

    new_node->next = head;
    head = new_node;
}

void print_list() {
    Node *curr = head;

    while (curr) {
        printf("%s\n", curr->data);
        curr = curr->next;
    }
}

void bubble_sort() {
    if (!head) return;
    int n = 0;
    Node *tmp = head;

    while (tmp) {
        n++;
        tmp = tmp->next;
    }

    for (int i = 0; i < n - 1; i++) {
        Node *curr = head;

        for (int j = 0; j < n - i - 1; j++) {

            if (strcmp(curr->data, curr->next->data) > 0) {
                char temp[81];
                strcpy(temp, curr->data);
                strcpy(curr->data, curr->next->data);
                strcpy(curr->next->data, temp);
            }

            curr = curr->next;
        }
    }
}

void *sort_thread(void *arg) {
    while (1) {
        sleep(5);

        pthread_mutex_lock(&mutex);
        bubble_sort();
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main() {
    pthread_t thread;
    pthread_mutex_init(&mutex, NULL);
    pthread_create(&thread, NULL, sort_thread, NULL);
    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), stdin)) {

        if (strcmp(buffer, "\n") == 0) {
            pthread_mutex_lock(&mutex);
            print_list();
            pthread_mutex_unlock(&mutex);
            continue;
        }

        int len = strlen(buffer);
        int pos = 0;

        while (pos < len) {

            char part[81];

            int chunk;
            if (len - pos > 80) {
                chunk = 80;
            } else {
                chunk = len - pos;
            }

            strncpy(part, buffer + pos, chunk);
            part[chunk] = '\0';

            pthread_mutex_lock(&mutex);
            push_front(part);
            pthread_mutex_unlock(&mutex);

            pos += chunk;
        }
    }

    pthread_join(thread, NULL);
    pthread_mutex_destroy(&mutex);

    return 0;
}