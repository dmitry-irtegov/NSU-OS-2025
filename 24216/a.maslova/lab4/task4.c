#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char* read_line() {
    char* line = NULL;
    size_t size = 0;
    int c;
    size_t len = 0;

    while ((c = getchar()) != '\n') {
        if (len >= size) {
            size = size == 0 ? 16 : size * 2;
            line = realloc(line, size);
            if (!line) {
                perror("Allocation failed");
                exit(EXIT_FAILURE);
            }
        }
        line[len++] = c;
    }

    if (line == NULL) {
        return NULL;
    }

    line = realloc(line, len + 1);
    line[len] = '\0';
    return line;
}

int main() {
    Node* head = NULL;
    Node* tail = NULL;

    while(1) {
        char* line = read_line();
        if (line == NULL) { 
            break;
        }

        if (line[0] == '.') {
            free(line);
            break;
        }

        Node* newNode = create_node(line);
        append_node(&head, &tail, newNode);
        free(line);
    }

    print_list(head);
    free_list(head);

    return 0;
}
