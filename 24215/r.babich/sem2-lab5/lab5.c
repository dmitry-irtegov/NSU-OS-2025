#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


void say_bye(void *arg) {
	printf("Bye!\n");
}


void *thread_printer(void *arg) {
	pthread_cleanup_push(say_bye, NULL);
	while (1) {
		puts(arg);
		sleep(1);
	}
	pthread_cleanup_pop(0);
	return NULL;
}


int main() {
	pthread_t thread;

	if (pthread_create(&thread, NULL, thread_printer, "Child thread") != 0) {
		fprintf(stderr, "Failed creating thread\n");
		exit(EXIT_FAILURE);
	}

	sleep(2);

	if (pthread_cancel(thread) != 0) {
		fprintf(stderr, "Failed canceling thread\n");
		exit(EXIT_FAILURE);
	}

	if (pthread_join(thread, NULL) != 0) {
		fprintf(stderr, "Failed joining thread\n");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
