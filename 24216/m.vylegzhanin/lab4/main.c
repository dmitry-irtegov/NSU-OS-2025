#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 1024

struct Node {
    char *line;
    struct Node *next;
};

int main(void) {
    struct Node *head = NULL, *tail = NULL;
    char buf[MAX_LINE_LEN];

    while (fgets(buf, sizeof(buf), stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
            --len;
        }
        if (buf[0] == '.') {
            break;
        }
        struct Node *node = malloc(sizeof(struct Node));
        if (!node) {
            perror("malloc failed");
            while (head) {
                struct Node *tmp = head->next;
                free(head->line);
                free(head);
                head = tmp;
            }
            return 1;
        }
        node->line = malloc(len + 1);
        if (!node->line) {
            perror("malloc failed");
            free(node);
            while (head) {
                struct Node *tmp = head->next;
                free(head->line);
                free(head);
                head = tmp;
            }
            return 1;
        }
        strcpy(node->line, buf);
        node->next = NULL;
        if (!head) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }

    for (struct Node *cur = head; cur; cur = cur->next) {
        printf("%s\n", cur->line);
    }

    while (head) {
        struct Node *tmp = head->next;
        free(head->line);
        free(head);
        head = tmp;
    }
    return 0;
}
