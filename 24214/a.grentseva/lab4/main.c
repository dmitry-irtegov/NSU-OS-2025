#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

typedef struct Node {
    char *data;
    struct Node *next;
} Node;

void print_list(const Node *head) {
    const Node *p = head;
    while (p) {
        printf("%s\n", p->data);
        p = p->next;
    }
}

void free_list(Node *head) {
    while (head) {
        Node *tmp = head->next;
        free(head->data);
        free(head);
        head = tmp;
    }
}

int main() {
    const size_t MAX_LEN = 512;

    Node *head = NULL;
    Node *tail = NULL;

    while (1) {
        size_t buffer_size = MAX_LEN;
        size_t total_read = 0;
        char *buffer = malloc(buffer_size);
        if (!buffer) {
            perror("Error malloc");
            return 1;
        }

        ssize_t len = 0;
        while (1) {
            len = read(STDIN_FILENO, buffer + total_read, 1);
            if (len <= 0) {
                break;  
            }
            if (buffer[total_read] == '\n') {
                break; 
            }
            total_read++;

            if (total_read + 1 >= buffer_size) {
                buffer_size *= 2;
                char *temp = realloc(buffer, buffer_size);
                if (!temp) {
                    free(buffer);
                    perror("Error realloc");
                    return 1;
                }
                buffer = temp;
            }
        }
        if (len <= 0 && total_read == 0) {
            free(buffer);
            break;
        }

        buffer[total_read] = '\0';

        if (buffer[0] == '.') {
            free(buffer);
            break; 
        }

        Node *node = malloc(sizeof(Node));
        if (!node) {
            perror("Error malloc");
            free(buffer);
            return 1;
        }
        node->data = buffer;
        node->next = NULL;

        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }

    print_list(head);
    free_list(head);
    return 0;
}

