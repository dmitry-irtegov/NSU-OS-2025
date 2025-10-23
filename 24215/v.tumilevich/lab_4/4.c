#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_LENGTH 1024

typedef struct Node {
    char *str;
    struct Node *next;
} Node;

Node* add_string(Node *head, const char *new_str) {
    if (new_str == NULL) return head;
    
    char *str_copy = (char *)malloc(strlen(new_str) + 1);
    if (str_copy == NULL) {
        perror("Failed to allocate memory for string");
        return head;
    }
    strcpy(str_copy, new_str);

    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Failed to allocate memory for node");
        free(str_copy);
        return head;
    }
    new_node->str = str_copy;
    new_node->next = NULL;

    if (head == NULL) {
        return new_node;
    } else {
        Node *current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
        return head;
    }
}

void print_list(Node *head) {
    Node *current = head;
    if (current == NULL) {
        printf("List is empty.\n");
        return;
    }
    printf("\nStrings in the list:\n");
    while (current != NULL) {
        printf("%s\n", current->str);
        current = current->next;
    }
}

void free_list(Node *head) {
    Node *current = head;
    while (current != NULL) {
        Node *next = current->next;
        free(current->str);
        free(current);
        current = next;
    }
}

char* read_full_line() {
    char buffer[MAX_INPUT_LENGTH];
    char *result = NULL;
    size_t total_len = 0;
    int is_complete = 0;

    while (!is_complete) {
        if (fgets(buffer, MAX_INPUT_LENGTH, stdin) == NULL) {
            break;
        }

        size_t chunk_len = strlen(buffer);
        char *new_result = realloc(result, total_len + chunk_len + 1);
        if (new_result == NULL) {
            perror("Failed to allocate memory for line");
            free(result);
            return NULL;
        }
        result = new_result;
        
        strcpy(result + total_len, buffer);
        total_len += chunk_len;

        if (chunk_len > 0 && buffer[chunk_len - 1] == '\n') {
            is_complete = 1;
            if (total_len > 0 && result[total_len - 1] == '\n') {
                result[total_len - 1] = '\0';
            }
        }
    }

    return result;
}

int main() {
    Node *head = NULL;

    while (1) {
        char *full_line = read_full_line();

        if (strcmp(full_line, ".") == 0) {
            free(full_line);
            break;
        }

        head = add_string(head, full_line);
        free(full_line);
    }

    print_list(head);
    free_list(head);

    return 0;
}