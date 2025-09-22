#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
	long LINE_MAX = sysconf(_SC_LINE_MAX);
	if (LINE_MAX == -1) {
		perror("Can not know the max line length\n");
		exit(1);
	}

	char *buffer = (char *)calloc(sizeof(char), LINE_MAX + 1);
	int new_line = 0;
	int finish_input = 0;
	if (buffer == NULL) {
		perror("Can not allocate memory for a buffer\n");
		exit(1);
	}

	char **strings = (char **)calloc(sizeof(char *), 4);
	size_t strings_len = 0;
	size_t strings_cap = 4;
	if (strings == NULL) {
		perror("Can not allocate memory for a strings list\n");
		free(buffer);
		exit(1);
	}

	while (fgets(buffer, LINE_MAX, stdin) != NULL) {
		char *temp = (char *)calloc(sizeof(char), strlen(buffer) + 1);
		if (temp == NULL) {
			perror("Can not allocate memory for a new list item\n");
			free(buffer);
			for (size_t i = 0; i < strings_len; i++) {
				free(strings[i]);
			}
			free(strings);
			exit(1);
		}
		strcpy(temp, buffer);
		if (strings_len >= strings_cap) {
			strings_cap *= 2;
			char **strings_new = (char **)realloc(strings, sizeof(char *) * strings_cap);
			if (strings_new == NULL) {
				perror("Can not reallocate memory for the list\n");
				free(buffer);
				for (size_t i = 0; i < strings_len; i++) {
					free(strings[i]);
				}
				free(strings);
				exit(1);
			}
			strings = strings_new;
		}
		strings[strings_len++] = temp;

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

	for (size_t i = 0; i < strings_len; i++) {
		printf("%s", strings[i]);
		free(strings[i]);
	}

	free(buffer);
	free(strings);
	exit(0);
}
