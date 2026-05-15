#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "node.h"

void initialize(singly_linked_list_t *list) {
	list->head = NULL;
	list->tail = NULL;
}

struct node_t* create_node(char *data) {
	struct node_t *node = malloc(sizeof(struct node_t));
	if (!node) {
		fprintf(stderr, "Memory allocation error\n");
		return NULL;
	}
	node->data = data;
	node->next = NULL;
	return node;
}

void insert_at_beggining(singly_linked_list_t *list, char *data) {
	char *dup = strdup(data);

	struct node_t *node = create_node(dup);

	if (!node) {
		free(dup);
		return;
	}
	node->next = list->head;
	list->head = node;
	if (!list->tail) {
		list->tail = node;
	}
}

void print(singly_linked_list_t *list) {
	struct node_t *node = list->head;
	while (node != NULL) {
		printf("%s -> ", node->data);
		node = node->next;
	}
}

void destroy(singly_linked_list_t *list) {
	node_t *current = list->head;
	while (current) {
  	node_t *tmp = current;
  	current = current->next;
    free(tmp->data);
    free(tmp);
  }
  list->head = NULL;
  list->tail = NULL;
}
