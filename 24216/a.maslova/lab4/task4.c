#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 1024

typedef struct Node {
    char* data;
    struct Node* next;
} Node;

Node* create_node(const char* str) {
    size_t len = strlen(str);
    char* new_str = (char*)malloc(len + 1);
    if (new_str == NULL) {
        perror("Allocation failed");
        exit(EXIT_FAILURE);
    }
    strcpy(new_str, str);
    
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        free(new_str);
        perror("Allocation failed");
        exit(EXIT_FAILURE);
    }
    
    newNode->data = new_str;
    newNode->next = NULL;
    
    return newNode;
}

void append_node(Node** head, Node** tail, Node* newNode) {
    if (*head == NULL) {
        *head = newNode;
        *tail = newNode;
    } else {
        (*tail)->next = newNode;
        *tail = newNode;
    }
}

void print_list(Node* head) {
    Node* current = head;
    while(current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }
}

void free_list(Node* head) {
    Node* current = head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp->data);
        free(temp);
    }
}

int main() {
    char buffer[BUFFER_SIZE];
    Node* head = NULL;
    Node* tail = NULL;

    while(1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        if (buffer[0] == '.') {
            break;
        }

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        
        Node* newNode = create_node(buffer);
        append_node(&head, &tail, newNode);
    }
    
    print_list(head);
    free_list(head);

    return 0;
}