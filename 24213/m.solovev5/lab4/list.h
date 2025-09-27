#ifndef LIST_H
#define LIST_H

#include <stddef.h>

typedef struct node_t {
	char *value;
	struct node_t *prev;
	struct node_t *next;
} node_t;

typedef struct list_t {
	node_t *head;
	node_t *tail;
	size_t size;
} list_t;


list_t *lcreate();

void lpushb(list_t *list, char *value);

char *lpopf(list_t *list);

void lfree(list_t *list);

#endif
