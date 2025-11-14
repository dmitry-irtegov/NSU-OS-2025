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
        printf("%s", p->data);
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

int main(void) {
    char temp_buf[1024];
    Node *head = NULL;
    Node *tail = NULL;
    int is_new_line = 1;

    while (fgets(temp_buf, sizeof(temp_buf), stdin) != NULL) {

        if (is_new_line && temp_buf[0] == '.') {
            break;
        }

        size_t len = strlen(temp_buf);

        if (is_new_line) {
            Node *node = malloc(sizeof(Node));
            if (!node) {
                perror("Error malloc");
                free_list(head);
                return 1;
            }

            node->data = malloc(len + 1);
            if (!node->data) {
                perror("Error malloc");
                free(node);
                free_list(head);
                return 1;
            }

            strcpy(node->data, temp_buf);
            node->next = NULL;

            if (!head) head = node; else tail->next = node;
            tail = node;
        } else {
            size_t old_len = strlen(tail->data);
            char *extended_data = realloc(tail->data, old_len + len + 1);
            if (!extended_data) {
                perror("Error realloc");
                free_list(head);
                return 1;
            }
            tail->data = extended_data;
            strcat(tail->data, temp_buf);
        }

        if (temp_buf[len - 1] == '\n') {
            is_new_line = 1;
        } else {
            is_new_line = 0;
        }
    }

    print_list(head);
    free_list(head);
    return 0;
}
