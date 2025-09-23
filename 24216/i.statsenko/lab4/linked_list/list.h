#pragma once
#include "node.h"

typedef struct list_s
{
    Node *begin;
    Node *end;
} List;

List listInit();
char listAppend(List *list, char *buf);
void listPrint(List *list);
void listDelete(List *list);