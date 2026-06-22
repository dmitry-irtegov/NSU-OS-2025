#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFERSIZE 1024

typedef struct node_t {
	struct node_t *next;
	char *value;
} node_t;

int main() {
	char buffer[BUFFERSIZE];
	int new_line = 1;
	int finish = 0;

	node_t *head = malloc(sizeof(node_t));
	if (head == NULL) {
		perror("Can not allocate memory for a list\n");
		exit(1);
	}
	node_t *tail = head;

	while (!finish && fgets(buffer, BUFFERSIZE, stdin) != NULL) {
		for (size_t i = 0; buffer[i] != '\0'; i++) {
			if (new_line && buffer[i] == '.') {
				finish = 1;
				buffer[i] = '\0';
			}
			new_line = buffer[i] == '\n';
		}

		tail->next = malloc(sizeof(node_t));
		if (tail->next == NULL) {
			perror("Can not allocate memory for a new list item\n");
			exit(1);
		} 
		tail = tail->next;
		tail->value = calloc(1, strlen(buffer) + 1);
		if (tail->value == NULL) {
			perror("Can not allocate memory for a line\n");
			exit(1);
		}
		strcpy(tail->value, buffer);
	}

	tail->next = NULL;
	node_t *item = head->next;
	free(head);
	
	while (item != NULL) {
		printf("%s", item->value);

		node_t *next = item->next;
		free(item->value);
		free(item);
		item = next;
	}

	exit(0);
}
