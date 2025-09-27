#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"

int main() {
	const size_t LINE_MAX = 1024;

	char *buffer = calloc(1, LINE_MAX + 1);
	int new_line = 0;
	int finish_input = 0;
	if (buffer == NULL) {
		perror("Can not allocate memory for a buffer\n");
		exit(1);
	}

	list_t *strings = lcreate();

	while (fgets(buffer, LINE_MAX, stdin) != NULL) {
		char *temp = calloc(1, strlen(buffer) + 1);
		if (temp == NULL) {
			perror("Can not allocate memory for a new list item\n");
			exit(1);
		}
		strcpy(temp, buffer);
		lpushb(strings, temp);

		for (size_t i = 0; buffer[i] != '\0'; i++) {
			if (new_line && buffer[i] == '.') {
				finish_input = 1;
			}
			new_line = buffer[i] == '\n';
		}

		if (finish_input && new_line) {
			break;
		}
	}

	while (strings->size) {
		char *temp = lpopf(strings);
		printf("%s", temp);
		free(temp);
	}

	free(buffer);
	lfree(strings);
	exit(0);
}
