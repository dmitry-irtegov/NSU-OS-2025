#include <stdlib.h>
#include <string.h>
#include "node.h"

Node *nodeCreate(char *buf)
{
    Node *node = malloc(sizeof(Node));
    node->next = NULL;
    if (node == NULL) {
        return NULL;
    }
    size_t len = strlen(buf);
    node->str = malloc(len);
    if (node->str == NULL) {
        free(node);
        return NULL;
    }
    strcpy(node->str, buf);
    return node;
}

void nodeDelete(Node *node)
{
    if (node != NULL) {
        free(node->str);
        free(node);
    }
}