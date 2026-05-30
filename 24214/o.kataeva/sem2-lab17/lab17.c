#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

typedef struct Node {
    struct Node* next;
    char sentence[BUFSIZ];
} Node;

pthread_mutex_t mutex;
Node* dummy_node = NULL;
int size = 0;

volatile int should_exit = 0; 

void add_in_begining(Node* new_node) {
    pthread_mutex_lock(&mutex);
    new_node->next = dummy_node->next;
    dummy_node->next = new_node;
    size++;
    pthread_mutex_unlock(&mutex);
}

void print_list() {
    pthread_mutex_lock(&mutex);
    Node* current = dummy_node->next;
    printf("Nodes in current state:\n");
    while(current) {
        printf("    %s\n", current->sentence);
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
}

void free_list() {
    pthread_mutex_lock(&mutex);
    Node* current = dummy_node; 
    while (current != NULL) {
        Node* next = current->next;
        free(current); 
        current = next;
    }
    dummy_node = NULL;
    size = 0;
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex); 
}

void* sort(void* param) {
    (void)param;
    while(!should_exit) {
        sleep(5);
        pthread_mutex_lock(&mutex);
        
        if(size == 0 || size == 1) {
            pthread_mutex_unlock(&mutex);
            continue;
        }

        printf("Thread started sort\n");
        for(int i = 0; i < size - 1; i++) {
            Node* current = dummy_node;
            for(int j = 0; j < size - 1 - i; j++) {
                Node* left = current->next;
                Node* right = left->next;
                if (strcmp(left->sentence, right->sentence) > 0) {
                    left->next = right->next;
                    right->next = left;
                    current->next = right;
                }
                current = current->next;
            }
        }
        printf("sorted\n");
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    dummy_node = (Node*)malloc(sizeof(Node));
    if (dummy_node == NULL) {
        perror("malloc dummy_node");
        return -1;
    }
    dummy_node->next = NULL;
    size = 0;
    
    char buffer[BUFSIZ];
    pthread_t thread;
    
    if (pthread_create(&thread, NULL, sort, NULL) != 0) {
        perror("pthread_create");
        free(dummy_node);
        pthread_mutex_destroy(&mutex);
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (buffer[0] == '\n') {
            print_list();
            continue;
        }
        
        size_t n = strcspn(buffer, "\n");
        buffer[n] = '\0';        
        Node* new_node = (Node*)malloc(sizeof(Node));
        if (new_node == NULL) {
            perror("malloc new_node");
            continue;
        }
        
        strcpy(new_node->sentence, buffer);
        add_in_begining(new_node);
    }

    should_exit = 1; 
    pthread_join(thread, NULL); 
    free_list(); 
    return 0;
}