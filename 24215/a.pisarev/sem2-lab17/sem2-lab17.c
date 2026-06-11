#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_CHUNK 80
#define MAX_LINE 1024

typedef struct Node {
    char data[MAX_CHUNK + 1];
    struct Node* next;
} Node;

Node* head = NULL;
pthread_mutex_t list_mutex;

void insert_at_head(const char* str) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        perror("Memory allocation");
        return;
    }
    strncpy(new_node->data, str, MAX_CHUNK);
    new_node->data[MAX_CHUNK] = '\0';
    new_node->next = head;
    head = new_node;
}

void print_list() {
    printf("Current status of the list:\n");
    if (!head) {
        printf("(the list is empty)\n");
    } else {
        Node* curr = head;
        int idx = 1;
        while (curr) {
            printf("%3d. [%s]\n", idx++, curr->data);
            curr = curr->next;
        }
    }
    printf("the end of the list\n\n");
}

void bubble_sort() {
    if (!head || !head->next) {
		return;
	}
    int swapped;
    do {
        swapped = 0;
        Node* curr = head;
        while (curr->next) {
            if (strcmp(curr->data, curr->next->data) > 0) {
                char temp[MAX_CHUNK + 1];
                strcpy(temp, curr->data);
                strcpy(curr->data, curr->next->data);
                strcpy(curr->next->data, temp);
                swapped = 1;
            }
            curr = curr->next;
        }
    } while (swapped);
}

void* sort_thread_func(void* arg) {
    while (1) {
        sleep(5);
        pthread_mutex_lock(&list_mutex);
        bubble_sort();
        pthread_mutex_unlock(&list_mutex);
    }
    return NULL;
}

void free_list() {
    Node* curr = head;
    while (curr) {
        Node* temp = curr;
        curr = curr->next;
        free(temp);
    }
    head = NULL;
}

int main() {
    pthread_mutex_init(&list_mutex, NULL);
    pthread_t sort_tid;
    if (pthread_create(&sort_tid, NULL, sort_thread_func, NULL) != 0) {
        perror("Creating thread");
        pthread_mutex_destroy(&list_mutex);
        return 1;
    }

    char line[MAX_LINE];
    printf("Enter the lines(blank line = show the list, Ctrl+D=Terminate):\n");

    while (fgets(line, MAX_LINE, stdin)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        if (len == 0) {
            pthread_mutex_lock(&list_mutex);
            print_list();
            pthread_mutex_unlock(&list_mutex);
            continue;
        }
        char* ptr = line;
        size_t remaining = len;
        while (remaining > 0) {
            size_t chunk_len = (remaining > MAX_CHUNK) ? MAX_CHUNK : remaining;
            char chunk[MAX_CHUNK + 1];
            memcpy(chunk, ptr, chunk_len);
            chunk[chunk_len] = '\0';

            pthread_mutex_lock(&list_mutex);
            insert_at_head(chunk);
            pthread_mutex_unlock(&list_mutex);

            ptr += chunk_len;
            remaining -= chunk_len;
        }
    }

    printf("\nTerminating...\n");
    
    pthread_mutex_destroy(&list_mutex);
    free_list();

    exit(0);
}
