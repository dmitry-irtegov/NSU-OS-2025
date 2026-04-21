#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "node.h"

#define BUFFER_SIZE	80

pthread_mutex_t node_mutex;

void *sorter_routine(void *arg) {
	node_t **node_ptr = arg;
	while (1) {
		pthread_mutex_lock(&node_mutex);
		node_sort(*node_ptr);
		pthread_mutex_unlock(&node_mutex);
		sleep(5);
	}
}

int main() {
	node_t *node = node_create();
	pthread_mutex_init(&node_mutex, NULL);
	pthread_t sorter_thread;
	int code = pthread_create(&sorter_thread, NULL, sorter_routine, &node);
	if (code != 0) {
		char buf[256];
		strerror_r(code, buf, sizeof buf);
		fprintf(stderr, "creating thread: %s\n", buf);
		exit(1);
	}

	char input_buf[BUFFER_SIZE];
	int overflow = 0;

	while (fgets(input_buf, BUFFER_SIZE, stdin)) {
		size_t len = strlen(input_buf);
		int next_overflow = 1;
		if (input_buf[len - 1] == '\n') {
			input_buf[len - 1] = '\0';
			next_overflow = 0;
		}
		
		pthread_mutex_lock(&node_mutex);
		if (len > 1) {
			node = node_put(node, input_buf);
		} else if (!overflow) {
			node_print(node);
		}
		pthread_mutex_unlock(&node_mutex);
		overflow = next_overflow;
	}

	code = pthread_cancel(sorter_thread);
	if (code != 0) {
		char buf[256];
		strerror_r(code, buf, sizeof buf);
		fprintf(stderr, "thread cancelation: %s\n", buf);
		exit(1);
	}
	code = pthread_join(sorter_thread, NULL);
	if (code != 0) {
		char buf[256];
		strerror_r(code, buf, sizeof buf);
		fprintf(stderr, "thread joining: %s\n", buf);
		exit(1);
	}
	code = pthread_mutex_destroy(&node_mutex);
	if (code != 0) {
		char buf[256];
		strerror_r(code, buf, sizeof buf);
		fprintf(stderr, "mutex destroying: %s\n", buf);
		exit(1);
	}
	node_destroy(node);
	return 0;
}
