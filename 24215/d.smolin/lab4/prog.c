#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct node_t {
    struct node_t *next;
    char *value;
} node_t;

void main() {
    char buffer[1024];
    node_t *head = NULL, *tail = NULL;

    while (fgets(buffer, sizeof buffer, stdin)) {
        if (buffer[0] == '.') break;

        size_t len = strlen(buffer);

        if (len && buffer[len - 1] == '\n') {
            buffer[--len] = '\0';
        }

        if (len == 0) continue;

        char *s = (char*)malloc(len + 1);

        if (!s) {
            perror("malloc string");
            exit(1);
        }

        memcpy(s, buffer, len + 1);

        node_t *n = (node_t*)malloc(sizeof(node_t));

        if (!n) {
            perror("malloc node");
            free(s);
            exit(1);
        }

        n->value = s; n->next = NULL;

        if (!head) head = tail = n;
        else {
            tail->next = n;
            tail = n;
        }
    }

    for (node_t *p = head; p; ) {
        printf("%s\n", p->value);
        node_t *q = p->next;
        free(p->value);
        free(p);
        p = q;
    }
    exit(0);
}
