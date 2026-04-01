#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

sem_t component_A;
sem_t component_B;
sem_t component_C;
sem_t module;

void handle_error(const char *message) {
	perror(message);
	exit(EXIT_FAILURE);
}

void handle_pthread_error(const char *message) {
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

void *produce_component_A(void *arg) {
	while (1) {
		printf("Producing component A\n");
		sleep(1);
		if (sem_post(&component_A) != 0) {
			handle_error("Failed to post semaphore component_A");
		}
	}
}

void *produce_component_B(void *arg) {
	while (1) {
		printf("Producing component B\n");
		sleep(2);
		if (sem_post(&component_B) != 0) {
			handle_error("Failed to post semaphore component_B");
		}
	}
}

void *produce_component_C(void *arg) {
	while (1) {
		printf("Producing component C\n");
		sleep(3);
		if (sem_post(&component_C) != 0) {
			handle_error("Failed to post semaphore component_C");
		}
	}
}

void *assemble_module(void *arg) {
	while (1) {
		if (sem_wait(&component_A) != 0) {
			handle_error("Failed to wait semaphore component_A");
		}
		if (sem_wait(&component_B) != 0) {
			handle_error("Failed to wait semaphore component_B");
		}
		printf("Assembled module\n");
		if (sem_post(&module) != 0) {
			handle_error("Failed to post semaphore module");
		}
	}
}

void *assemble_widget(void *arg) {
	while (1) {
		if (sem_wait(&module) != 0) {
			handle_error("Failed to wait semaphore module");
		}
		if (sem_wait(&component_C) != 0) {
			handle_error("Failed to wait semaphore component_C");
		}
		printf("Assembled widget\n");
	}
}

int main() {

	pthread_t producer_A;
	pthread_t producer_B;
	pthread_t producer_C;
	pthread_t producer_module;

	if (sem_init(&component_A, 0, 0) != 0) {
		handle_error("Failed to init semaphore component_A");
	}
	if (sem_init(&component_B, 0, 0) != 0) {
		handle_error("Failed to init semaphore component_B");
	}
	if (sem_init(&component_C, 0, 0) != 0) {
		handle_error("Failed to init semaphore component_C");
	}
	if (sem_init(&module, 0, 0) != 0) {
		handle_error("Failed to init semaphore module");
	}
	
	if (pthread_create(&producer_A, NULL, produce_component_A, NULL) != 0) {
		handle_pthread_error("Failed creating thread producer_A");
	}
	if (pthread_create(&producer_B, NULL, produce_component_B, NULL) != 0) {
		handle_pthread_error("Failed creating thread producer_B");
	}
	if (pthread_create(&producer_C, NULL, produce_component_C, NULL) != 0) {
		handle_pthread_error("Failed creating thread producer_C");
	}
	if (pthread_create(&producer_module, NULL, assemble_module, NULL) != 0) {
		handle_pthread_error("Failed creating thread producer_module");
	}

	assemble_widget(NULL);

	exit(EXIT_SUCCESS);
}
