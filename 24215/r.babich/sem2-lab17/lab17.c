#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "sort.h"

#define MAX_LEN 80
pthread_mutex_t mutex;

typedef struct {
	singly_linked_list_t *list;
	int *running;
} thread_args_t;

void process_input(char *input, singly_linked_list_t *list) {
	int len = strlen(input);
	int index = 0;
	while (index < len) {
		char buffer[MAX_LEN + 1];
		memset(buffer, 0, sizeof(buffer));
		strncpy(buffer, input+index, MAX_LEN);
		buffer[MAX_LEN]  = '\0';
		pthread_mutex_lock(&mutex);
		insert_at_beggining(list, buffer);
		pthread_mutex_unlock(&mutex);
		index += MAX_LEN;
	}
}

void *sort_list(void *arg) {
	thread_args_t *args = (thread_args_t*)arg;
	while (*args->running) {
		pthread_mutex_lock(&mutex);
		bubble_sort(args->list);
		pthread_mutex_unlock(&mutex);
		sleep(5);
	}
	return NULL;
}

int main(int argc, char **argv) {
	singly_linked_list_t list;
	initialize(&list);
	pthread_t thread;
	int running = 1;
	thread_args_t args;
	args.list = &list;
	args.running = &running;
	pthread_mutex_init(&mutex, NULL);
	pthread_create(&thread, NULL, sort_list, &args);
	char input[128];
	while (1) {
		if (!fgets(input, sizeof(input), stdin)) {
			*args.running = 0;
			break;
		}
	 	input[strcspn(input, "\n")] = '\0';
		if (strlen(input) == 0) {
			pthread_mutex_lock(&mutex);
			print(&list);
			pthread_mutex_unlock(&mutex);
		} else {
			process_input(input, &list);
		}
	}

	pthread_join(thread, NULL);
	destroy(&list);
	pthread_mutex_destroy(&mutex);

	return 0;
}
