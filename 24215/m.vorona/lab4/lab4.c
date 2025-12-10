#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

struct node {
    char *str;
    struct node *next;
};

void free_list(struct node *head) {
    struct node *current = head;
    while (current != NULL) {
        struct node *next = current->next;
        free(current->str);
        free(current);
        current = next;
    }
}

int main(void) {
    struct node *head = NULL;
    struct node *tail = NULL;
    struct node *new_node;
    char buf[MAX_LINE];
    size_t len;

    printf("Enter lines (terminate with . at start of line):\n");

    while (fgets(buf, MAX_LINE, stdin) != NULL) {
        len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') {
            buf[len-1] = '\0';
            len--;
        }
        if (len > 0 && buf[0] == '.') {
            break;
        }

        new_node = malloc(sizeof(struct node));
        if (new_node == NULL) {
            perror("malloc node");
            free_list(head);
            exit(1);
        }

        new_node->str = malloc(len + 1);
        if (new_node->str == NULL) {
            perror("malloc string");
            free(new_node);
            free_list(head);
            exit(1);
        }

        strcpy(new_node->str, buf);
        new_node->next = NULL;

        if (head == NULL) {
            head = tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
    }

    printf("\nList contents:\n");
    for (struct node *current = head; current != NULL; current = current->next) {
        printf("%s\n", current->str);
    }

    free_list(head);
    return 0;
}
