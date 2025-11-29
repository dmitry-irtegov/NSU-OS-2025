#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *str;
    struct Node *next;
};

int main() {
    const int MAXLINE = 1024;
    char buffer[MAXLINE];

    struct Node *head = NULL;
    struct Node *tail = NULL;

    while (1) {
        printf("> ");
        if (fgets(buffer, MAXLINE, stdin) == NULL)
            break;

        if (buffer[0] == '.')
            break;

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        char *s = (char *)malloc(len + 1);
        if (!s) {
            perror("malloc");
            return 1;
        }
        memcpy(s, buffer, len + 1);

        struct Node *node = (struct Node *)malloc(sizeof(struct Node));
        if (!node) {
            perror("malloc");
            return 1;
        }
        node->str = s;
        node->next = NULL;

        if (tail == NULL) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }

    printf("\n--- LIST ---\n");
    struct Node *cur = head;
    while (cur) {
        printf("%s\n", cur->str);
        cur = cur->next;
    }

    cur = head;
    while (cur) {
        struct Node *tmp = cur->next;
        free(cur->str);
        free(cur);
        cur = tmp;
    }

    return 0;
}

