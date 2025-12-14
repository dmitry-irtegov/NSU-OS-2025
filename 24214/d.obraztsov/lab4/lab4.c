#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

struct Node {
    char* str;
    struct Node* next;
};

int main() {
    char buffer[BUFFER_SIZE];
    struct Node* head = NULL;
    struct Node* current = NULL;
    struct Node* new_node = NULL;
        
    while (1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        if (buffer[0] == '.') {
            break;
        }
        
        new_node = (struct Node*) malloc(sizeof(struct Node));
        if (!new_node) {
            fprintf(stderr, "Не удалось выделить память для узла\n");
            while (head) {
                struct Node* temp = head;
                head = head->next;
                free(temp->str);
                free(temp);
            }
            return 1;
        }
        
        size_t len = strlen(buffer);
        new_node->str = (char*) malloc(len + 1);
        if (!new_node->str) {
            fprintf(stderr, "Не удалось выделить память для строки\n");
            free(new_node);
            while (head) {
                struct Node* temp = head;
                head = head->next;
                free(temp->str);
                free(temp);
            }
            return 1;
        }
        
        strcpy(new_node->str, buffer);
        new_node->next = NULL;
        
        if (!head) {
            head = new_node;
            current = head;
        } else {
            current->next = new_node;
            current = new_node;
        }
    }
    
    current = head;
    while (current) {
        printf("%s", current->str);
        current = current->next;
    }
    
    current = head;
    while (current) {
        struct Node* next = current->next;
        free(current->str);
        free(current);
        current = next;
    }
    
    return 0;
}