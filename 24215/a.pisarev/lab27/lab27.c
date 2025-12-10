#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]) {

	if (argc<2) {
		fprintf(stderr, "Filename is missing\n");
		return 1;
	}

	FILE *f = fopen(argv[1], "r");
	if (f == NULL) {
		perror("fopen");
		return 1;
	}


	FILE *p = popen("wc -l", "w");
	if (p == NULL) {
		perror("popen");
		fclose(f);
		return 1;
	}

	char *line = NULL;
	size_t len = 0;

	while (1) {
		if (getline(&line, &len, f) == -1)
			break;
		if (strcmp(line, "\n") == 0)
		fputc('\n', p);
	}

	if (line!=NULL){
		free(line);
	}
	fclose(f);
	pclose(p);
	return 0;
}
