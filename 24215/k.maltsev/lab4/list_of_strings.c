#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

struct node {
    char *data;
    struct node *next;
};

struct node* add_to_list(struct node *head, const char *str) {
    struct node *new_node;
    struct node *current;
    size_t str_len;
    
    new_node = (struct node *)malloc(sizeof(struct node));
    if (new_node == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for node\n");
        return head;
    }
    
    str_len = strlen(str);
    if (str_len > 0 && str[str_len - 1] == '\n') {
        str_len--;
    }
    
    new_node->data = (char *)malloc(str_len + 1);
    if (new_node->data == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for string\n");
        free(new_node);
        return head;
    }
    
    strncpy(new_node->data, str, str_len);
    new_node->data[str_len] = '\0';
    new_node->next = NULL;
    
    if (head == NULL) {
        return new_node;
    }
    
    current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;
    
    return head;
}

void print_list(struct node *head) {
    struct node *current = head;
    
    printf("\nStored strings:\n");
    printf("---------------\n");
    
    if (current == NULL) {
        printf("(empty list)\n");
        return;
    }
    
    while (current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }
}

void free_list(struct node *head) {
    struct node *current = head;
    struct node *next;
    
    while (current != NULL) {
        next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
}

int main(void) {
    char buffer[MAX_LINE_LENGTH];
    struct node *head = NULL;
    
    printf("Enter strings (type '.' on a new line to finish):\n");
    
    while (1) {
        if (fgets(buffer, MAX_LINE_LENGTH, stdin) == NULL) {
            break;
        }
        
        if (buffer[0] == '.' && (buffer[1] == '\n' || buffer[1] == '\0')) {
            break;
        }
        
        head = add_to_list(head, buffer);
    }
    
    print_list(head);
    
    free_list(head);
    
    return 0;
}
