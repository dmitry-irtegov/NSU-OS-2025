#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "node.h"

void initialize(singly_linked_list_t *list) {
	list->head = NULL;
	list->tail = NULL;
	pthread_mutex_init(&list->mutex, NULL);
}

struct node_t* create_node(char *data) {
	struct node_t *node = malloc(sizeof(struct node_t));
	if (!node) {
		fprintf(stderr, "Memory allocation error\n");
		return NULL;
	}
	node->data = data;
	if (!node->data) {
		free(node);
		return NULL;
	}
	pthread_mutex_init(&node->mutex, NULL);
	node->next = NULL;
	return node;
}

void insert_at_beggining(singly_linked_list_t *list, char *data) {
	struct node_t *node = create_node(strdup(data));
	if (!node) {
		return;
	}
	pthread_mutex_lock(&list->mutex);
	node->next = list->head;
	list->head = node;
	if (!node->next) {
		list->tail = node;
	}
	pthread_mutex_unlock(&list->mutex);
}

void print(singly_linked_list_t *list) {
	pthread_mutex_lock(&list->mutex);
	struct node_t *node = list->head;
	while (node != NULL) {
		if (node->data) {
			printf("%s -> ", node->data);
		} else {
			printf("(null) -> ");
		}
		node = node->next;
	}
	printf("\n");
	pthread_mutex_unlock(&list->mutex);
}

void destroy(singly_linked_list_t *list) {
	node_t *current = list->head;
	while (current) {
  	node_t *tmp = current;
  	current = current->next;
		pthread_mutex_destroy(&tmp->mutex);
		free(tmp->data);
    free(tmp);
  }
	pthread_mutex_destroy(&list->mutex);
  list->head = NULL;
}
