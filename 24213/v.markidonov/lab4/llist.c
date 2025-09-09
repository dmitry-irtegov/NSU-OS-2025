#include <stdlib.h>
#include "llist.h"

void llist_init(llist* list) {
    list->first = NULL;
    list->last = NULL;
}

int llist_push(llist* list, char* str) {
    listelem *new;
    if (!(new = (listelem*)malloc(sizeof(listelem)))) {
        return -1;
    }

    new->next = NULL;
    new->str = str;

    if (list->first == NULL) {
        list->first = new;
        list->last = new;
        return 0;
    }

    list->last->next = new;
    list->last = new;
    return 0;
}

char* llist_pop(llist* list) {
    listelem *first = list->first;

    if (first == NULL) {
        return NULL;
    }

    list->first = first->next;
    if (list->first == NULL) {
        list->last = NULL;
    }

    char *str = first->str;
    free(first);
    return str;
}

void llist_free(llist* list) {
    listelem *cur = list->first;
    while (cur != NULL) {
        listelem* next = cur->next;
        free(cur->str);
        free(cur);
        cur = next;
    }
}
