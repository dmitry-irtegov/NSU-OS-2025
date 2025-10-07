#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char *data;
    struct Node *next;
} Node;

Node *createNode(char *data);
Node *insertNode(Node *tail, char *data);
void printList(Node *head);
void freeList(Node *head);

#endif
