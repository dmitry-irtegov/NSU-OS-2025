#pragma once

typedef struct node_s
{
    char *str;
    struct node_s* next;
} Node;

Node *nodeCreate(char *buf);
void nodeDelete(Node *node);