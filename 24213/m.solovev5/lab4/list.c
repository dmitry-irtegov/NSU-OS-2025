#include <stdlib.h>

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


static node_t *ncreate(char *value, node_t *prev, node_t *next) {
	node_t *node = malloc(sizeof(node_t));
	node->value = value;
	node->prev = prev;
	node->next = next;

	return node;
}

static void npush(char *value, node_t *prev) {
	node_t *next = prev->next;
	node_t *node = ncreate(value, prev, next);
	prev->next = node;
	next->prev = node;
}

static char *npop(node_t *node) {
	node_t *prev = node->prev;
	node_t *next = node->next;
	char *value = node->value;

	prev->next = next;
	next->prev = prev;
	free(node);

	return value;
}

list_t *lcreate() {
	list_t *list = malloc(sizeof(list_t));
	list->head = ncreate(NULL, NULL, NULL);
	list->head->next = list->head;
	list->head->prev = list->head;
	list->tail = list->head;
	list->size = 0;

	return list;
}

void lpushb(list_t *list, char *value) {
	npush(value, list->tail->prev);
	list->size++;
}

char *lpopf(list_t *list) {
	char *value = npop(list->head->next);
	list->size--;

	return value;
}

void lfree(list_t *list) {
	while (list->size) {
		char *temp = lpopf(list);
		free(temp);
	}

	free(list->head);
	free(list);
}
