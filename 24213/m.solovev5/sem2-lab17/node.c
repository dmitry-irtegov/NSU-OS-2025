#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "node.h"

node_t *node_create() {
	node_t *node = malloc(sizeof(node_t));

	node->value = NULL;
	node->next = NULL;
	return node;
}

void node_destroy(node_t *node) {
	node_t *current_node = node;

	while (current_node != NULL) {
		node_t *next_node = current_node->next;

		free(current_node->value);
		free(current_node);
		current_node = next_node;
	}
}

node_t *node_put(node_t *node, char *value) {
	char *copy_value = strdup(value);
	node_t *new_node = node_create();

	new_node->value = copy_value;
	new_node->next = node;
	return new_node;
}

void node_print(node_t *node) {
	printf("List:\n");
	node_t *current_node = node;

	while (current_node != NULL) {
		if (current_node->value != NULL) {
			printf("\t%s;\n", current_node->value);
		}
		current_node = current_node->next;
	}
}

size_t node_len(node_t *node) {
	if (node == NULL) {
		return 0;
	}

	size_t len = 0;
	while (node->next != NULL) {
		len++;
		node = node->next;
	}
	return len;
}

void node_sort(node_t *node) {
	if (node == NULL) {
		return;
	}

	int swapped;
	node_t *bubble;
	do {
		swapped = 0;
		bubble = node;
		while (bubble->next != NULL && bubble->next->value != NULL) {
			if (strcmp(bubble->value, bubble->next->value) > 0) {
				char * tmp = bubble->value;
				bubble->value = bubble->next->value;
				bubble->next->value = tmp;
				swapped = 1;
			}
			bubble = bubble->next;
		}
	} while (swapped);
}
