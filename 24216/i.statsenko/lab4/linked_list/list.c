#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

List listInit()
{
    List list;
    list.begin = NULL;
    list.end = NULL;
    return list;
}

char listAppend(List *list, char *buf)
{
    Node *node = nodeCreate(buf);
    if (node == NULL) {
        return 0;
    }
    if (list->begin == NULL) {
        list->begin = node;
        list->end = list->begin;
    } else {
        list->end->next = node;
        list->end = node;
    }
    return 1;
}

void listPrint(List *list)
{
    for (Node *node = list->begin; node != NULL; node = node->next) {
        printf("%s", node->str);
    }
}

void listDelete(List *list)
{
    for (Node *node = list->begin; node != list->end;) {
        Node *next = node->next;
        nodeDelete(node);
        node = next;
    }
}