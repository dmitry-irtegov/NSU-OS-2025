#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHILD_NUMBER 4

void *child_thread(void *args) {
	char **message = (char **) args;
	for (int i = 0; message[i] != NULL; i++) {
		printf("%s", *message);
	}

	return 0;
}

int main() {
	pthread_t threads[CHILD_NUMBER];
	char *messages[][4] = { 
		{"Hello from thread 1\n", "Bye from thread 1\n", NULL},
		{"Hello from thread 2\n", "Another message from thread 2\n", "Bye from thread 2\n", NULL},
		{"Hello from thread 3\n", "Bye from thread 3\n", NULL},
		{"Hello from thread 4\n", "Bye from thread 4\n", NULL},
	};

	for (int i = 0; i < CHILD_NUMBER; i++) {
		int code = pthread_create(&threads[i], NULL, child_thread, messages[i]);

		if (code != 0) {
			char buf[256];
			strerror_r(code, buf, sizeof(buf));
			fprintf(stderr, "Error creating thread: %s", buf);
			exit(1);
		}
	}

	for (int i = 0; i < CHILD_NUMBER; i++) {
		int code = pthread_join(threads[i], NULL);

		if (code != 0) {
			char buf[256];
			strerror_r(code, buf, sizeof(buf));
			fprintf(stderr, "Error joining thread: %s", buf);
			exit(1);
		}
	}
	
	return 0;
}
