#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

#define MAX_LEN_LIST 100

int main() {

	srand(time(NULL));
	int sorted_list[MAX_LEN_LIST] = {0};
	int i;

    for (i = 0; i < MAX_LEN_LIST; i++) {
		sorted_list[i] = rand() % 100;
		if (i % 10 == 0)
			printf("\n");
		printf("%d ", sorted_list[i]);
	}
	printf("\n");

	FILE *fd[2];

	if (p2open("sort -n", fd) == -1) {
		fprintf(stderr, "p2open failed");
		return 1;
	}

	for (i = 0; i < MAX_LEN_LIST; i++) {
		fprintf(fd[0], "%d\n", sorted_list[i]);
	}

	fclose(fd[0]);
	for (i = 0; i < MAX_LEN_LIST; i++) {
		fscanf(fd[1], "%d", &sorted_list[i]);

		if (i % 10 == 0)
			printf("\n");
		printf("%d ", sorted_list[i]);
	}
	printf("\n");

    if (p2close(fd) == -1) {
		fprintf(stderr, "p2close failed");
		return 1;
	}

	return 0;
}
