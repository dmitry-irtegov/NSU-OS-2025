#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 1024

typedef struct ListNode {
    char *line;
    struct ListNode *next;
} ListNode;

ListNode *node_create(const char *src) {
    size_t len = strlen(src);
    char *copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, src, len + 1);

    ListNode *node = (ListNode *)malloc(sizeof(ListNode));
    if (node == NULL) {
        free(copy);
        return NULL;
    }

    node->line = copy;
    node->next = NULL;

    return node;
}

void list_append(ListNode **head, ListNode **tail, ListNode *node) {
    if (*head == NULL) {
        *head = node;
        *tail = node;
    } else {
        (*tail)->next = node;
        *tail = node;
    }
}

void list_print(const ListNode *head) {
    const ListNode *cur = head;
    while (cur != NULL) {
        printf("%s", cur->line);
        cur = cur->next;
    }
}

void list_free(ListNode *head) {
    while (head != NULL) {
        ListNode *tmp = head;
        head = head->next;
        free(tmp->line);
        free(tmp);
    }
}

int main(void) {
    ListNode *head = NULL;
    ListNode *tail = NULL;
    char buffer[MAX_LINE_LEN];

    for (;;) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        if (buffer[0] == '.') {
            break;
        }

        ListNode *node = node_create(buffer);
        if (node == NULL) {
            perror("memory allocation failed");
            list_free(head);
            exit(EXIT_FAILURE);
        }

        list_append(&head, &tail, node);
    }

    list_print(head);
    list_free(head);

    return 0;
}
