#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef struct Node_t {
    char data[81];
    struct Node_t* next;
    struct Node_t* prev;
} Node;

Node* head = NULL;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;

void append(const char* str) {
    if (strlen(str) <= 80) {
        Node* node = malloc(sizeof(Node));
        node->prev = NULL;
        node->next = head;
        strcpy(node->data, str);

        if (head != NULL) head->prev = node;
        head = node;
        return;
    }

    char base[81];
    strncpy(base, str, 80);
    base[80] = '\0';
    const char* part = &str[80];
    append(base);
    append(part);
}

void print_list() {
    int n = 0;
    for (Node* i = head; i != NULL; i = i->next) {
        printf("%d --- [%s]\n", n++, i->data);
    }
}

void free_list() {
    if (head == NULL) return;

    Node* i = head;
    while (i != NULL) {
        Node* next = i->next;
        free(i);
        i = next;
    }

    head = NULL;
}

void parse() {
    char buffer[1024];
    fgets(buffer, sizeof(buffer), stdin);

    if (strcmp(buffer, "\n") == 0) {
        pthread_mutex_lock(&list_mutex);
        print_list();
        pthread_mutex_unlock(&list_mutex);
    } else {
        buffer[strcspn(buffer, "\n")] = '\0';
        pthread_mutex_lock(&list_mutex);
        append(buffer);
        pthread_mutex_unlock(&list_mutex);
    }
}

void sort(Node* head) {
    for (Node* i = head; i != NULL; i = i->next)
        for (Node* j = head; j->next != NULL; j = j->next)
            if (strcmp(j->data, j->next->data) > 0) {
                char tmp[81];
                strcpy(tmp, j->data);
                strcpy(j->data, j->next->data);
                strcpy(j->next->data, tmp);
            }
}

void* sort_thread(void* arg) {
    while (1) {
        struct timespec ts = {.tv_sec = 5, .tv_nsec = 0};
        nanosleep(&ts, NULL);

        pthread_mutex_lock(&list_mutex);
        sort(head);
        pthread_mutex_unlock(&list_mutex);
    }

    return NULL;
}

int main() {
    pthread_t tid;
    pthread_create(&tid, NULL, sort_thread, NULL);

    while (1) parse();

    pthread_join(tid, NULL);
    free_list();
    pthread_mutex_destroy(&list_mutex);

    return 0;
}