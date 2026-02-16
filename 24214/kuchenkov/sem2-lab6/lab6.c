#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

void* thread_function(void* arg) {
	char *a = (char*)arg;
	usleep(strlen(a) * 100000);
	printf("%s", a);
	free(a);
	return NULL;
}

int main() {
	int n = 0;
	pthread_t thread[100];
	char buffer[1024];

	while (n < 100 && fgets(buffer, sizeof(buffer), stdin) != NULL) {
		char *copy = strdup(buffer);
		int code = pthread_create(&thread[n], NULL, thread_function, copy);
		if (code != 0) {
			fprintf(stderr, "Failed to create a thread");
			return 1;
		}
		n++;
	}
	for (int i = 0; i < n; i++) {
		int code = pthread_join(thread[i], NULL);
		if (code != 0) {
			fprintf(stderr, "Failed to join a thread");
			return 1;
		}
	}
	return 0;
}
