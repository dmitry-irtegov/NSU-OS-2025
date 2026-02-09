#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void *child_routine(void *param) {
	for (int i = 0; i < 10; i++) {
		printf("Line %2d from child thread\n", i + 1);
	}

	return 0;
}

int main(int argc, char *argv[]) {
	pthread_t child_thread;
	int code = pthread_create(&child_thread, NULL, child_routine, NULL);

	if (code != 0) {
		char buf[256];
		strerror_r(code, buf, sizeof buf);
		fprintf(stderr, "%s: creating thread: %s\n", argv[0], buf);
		exit(1);
	}

	for (int i = 0; i < 10; i++) {
		printf("Line %2d from main thread\n", i + 1);
	}

	pthread_exit(0);
}
