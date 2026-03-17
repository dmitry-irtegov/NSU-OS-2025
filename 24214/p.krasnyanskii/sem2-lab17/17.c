#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LEN 80

typedef struct Node {
    char data[MAX_LEN + 1];
    struct Node* next;
} Node;

Node* head = NULL;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int running = 1;

void add_to_list(char* str) {
    Node* new_node = malloc(sizeof(Node));
    if (!new_node) { perror("malloc"); return; }
    strcpy(new_node->data, str);

    pthread_mutex_lock(&list_mutex);
    new_node->next = head;
    head = new_node;
    pthread_mutex_unlock(&list_mutex);
}

void print_list() {
    pthread_mutex_lock(&list_mutex);
    Node* current = head;
    printf("\n--- LIST ---\n");
    while (current) {
        printf("%s\n", current->data);
        current = current->next;
    }
    printf("------------\n");
    pthread_mutex_unlock(&list_mutex);
}

void sort_list() {
    if (!head) return;
    int swapped;
    do {
        swapped = 0;
        Node* current = head;
        while (current->next) {
            if (strcmp(current->data, current->next->data) > 0) {
                char temp[MAX_LEN + 1];
                strcpy(temp, current->data);
                strcpy(current->data, current->next->data);
                strcpy(current->next->data, temp);
                swapped = 1;
            }
            current = current->next;
        }
    } while (swapped);
}

void* sorter_thread(void* arg) {
    while (running) {
        sleep(5);
        pthread_mutex_lock(&list_mutex);
        sort_list();
        pthread_mutex_unlock(&list_mutex);
        printf("List sorted\n");
    }
    return NULL;
}

void free_list() {
    Node* current = head;
    while (current) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    head = NULL;
}

int main() {
    pthread_t sorter;
    pthread_create(&sorter, NULL, sorter_thread, NULL);

    char buffer[1024];
    while (1) {
        if (!fgets(buffer, sizeof(buffer), stdin))
            break;
        int len = strlen(buffer);
        if (buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';
        if (strlen(buffer) == 0) {
            print_list();
            continue;
        }
        char* ptr = buffer;
        while (strlen(ptr) > MAX_LEN) {
            char temp[MAX_LEN + 1];
            strncpy(temp, ptr, MAX_LEN);
            temp[MAX_LEN] = '\0';
            add_to_list(temp);
            ptr += MAX_LEN;
        }
        add_to_list(ptr);
    }

    running = 0;
    pthread_join(sorter, NULL);

    free_list();
    pthread_mutex_destroy(&list_mutex);
    return 0;
}

