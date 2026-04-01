#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define num_steps 200000000

int thread_amount;

void *calculate(void *arg) {
	int id = *(int*)arg;

	double *sum = malloc(sizeof(double));
	*sum = 0;
	for (int i = id; i < num_steps; i += thread_amount) {
		*sum += 1.0/(i*4.0 + 1.0);
    *sum -= 1.0/(i*4.0 + 3.0);
	}

	pthread_exit(sum);
	return NULL;
}

int main(int argc, char** argv) {
    
	if (argc > 1) {
		thread_amount = atoi(argv[1]);
	} else {
		fprintf(stderr, "You need to specify thread amount\n");
		exit(EXIT_FAILURE);
	}

	pthread_t threads[thread_amount];
	int tids[thread_amount];

	for (int i = 0; i < thread_amount; i++) {
		tids[i] = i;
		if (pthread_create(&threads[i], NULL, calculate, &tids[i]) != 0) {
			fprintf(stderr, "Failed creating thread\n");
			exit(EXIT_FAILURE);
		}
	}

	double pi = 0;
	for (int i = 0; i < thread_amount; i++) {
		double *part;
		if (pthread_join(threads[i], (void**)&part) != 0) {
			fprintf(stderr, "Failed joining thread\n");
			exit(EXIT_FAILURE);
		}
		pi += *part;
		free(part);
	}
		
	pi = pi * 4.0;
	printf("pi done - %.15g \n", pi);      
	exit(EXIT_SUCCESS);
}

