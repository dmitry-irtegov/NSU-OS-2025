#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    char *buffer = malloc(MAX_LEN);
    if (!buffer) {
        perror("Error malloc");
        return 1;
    }

    Node *head = NULL;
    Node *tail = NULL;

    while (fgets(buffer, MAX_LEN, stdin)) {
        size_t len = strlen(buffer);
        
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        if (buffer[0] == '.') {
            break;
        }

        if (len >= MAX_LEN - 1) {
            size_t buffer_size = MAX_LEN;
            while (len >= buffer_size - 1) {
                buffer_size *= 2;
            }
            char *temp = realloc(buffer, buffer_size);
            if (!temp) {
                perror("Error realloc");
                free(buffer);
                free_list(head);
                return 1;
            }
            buffer = temp;
            
            while (fgets(buffer + len, buffer_size - len, stdin)) {
                size_t read_len = strlen(buffer + len);
                len += read_len;
                if (buffer[len - 1] == '\n') {
                    buffer[len - 1] = '\0';
                    break;
                }
                if (read_len < buffer_size - len - 1) {
                    break; 
                }
                buffer_size *= 2;
                temp = realloc(buffer, buffer_size);
                if (!temp) {
                    perror("Error realloc");
                    free(buffer);
                    free_list(head);
                    return 1;
                }
                buffer = temp;
            }
        }

        Node *node = malloc(sizeof(Node));
        if (!node) {
            perror("Error malloc");
            free(buffer);
            free_list(head);
            return 1;
        }
        node->data = strdup(buffer);  
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
    free(buffer);
    return 0;
}
