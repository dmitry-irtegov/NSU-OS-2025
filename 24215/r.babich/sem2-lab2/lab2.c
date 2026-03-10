#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"


void *thread_printer(void *arg) {
	for (int i = 0; i < 10; i++) {
		puts(arg);
	}
	return NULL;
}


int main() {
	pthread_t thread;

	if (pthread_create(&thread, NULL, thread_printer, "Child thread") != 0) {
		fprintf(stderr, "Failed creating thread\n");
		exit(EXIT_FAILURE);
	}

	if (pthread_join(thread, NULL) != 0) {
		fprintf(stderr, "Failed joining thread\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < 10; i++) {
		puts("Parent thread");
	}
}

