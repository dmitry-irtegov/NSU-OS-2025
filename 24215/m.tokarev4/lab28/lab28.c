#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <time.h>

int main() {
	srand(time(NULL));
	FILE* pipefd[2];
	int number;
	int cntread = 0;

	if ((p2open("sort -n", pipefd)) == -1) {
		perror("p2popen failed");
		return EXIT_FAILURE;
	}

	for (int i = 0; i < 100; i++) {
		fprintf(pipefd[0], "%d\n", rand() % 100);
	}

	fclose(pipefd[0]);

	while ((fscanf(pipefd[1], "%d", &number)) != EOF) {
		if (cntread % 10 == 9) {
			printf("%d\n", number);
		}
		else {
			printf("%d ", number);
		}
		cntread++;
	}

	fclose(pipefd[1]);

	return 0;
}
