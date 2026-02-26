#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define num_steps 200000000

int num_threads;

void* compute_part(void* param) {
	int index = *(int*)param;
	double part_res = 0.0;

	for (int i = index; i < num_steps; i += num_threads) {
		part_res += 1.0/(i*4.0 + 1.0);
		part_res -= 1.0/(i*4.0 + 3.0);
	}
	double* result = malloc(sizeof(double));
	*result = part_res;
	pthread_exit((void*)result);

}
int main(int argc, char** argv) {
	if (argc != 2) return 1;

	num_threads = atoi(argv[1]);
	if (num_threads == 0) {
		exit(EXIT_FAILURE);
	}
	
	double res = 0.0;
	pthread_t* threads = malloc(num_threads*sizeof(pthread_t));
	int* indixes = malloc(num_threads*sizeof(int));
	for (int i = 0; i < num_threads; i++) {
		indixes[i] = i;
		int result = pthread_create(&threads[i], NULL, compute_part, &indixes[i]);
		if (result != 0) {
			char buf[256];
			strerror_r(result, buf, sizeof(buf));
			fprintf(stderr, "error creating thread: %s\n", buf);
			exit(EXIT_FAILURE);
		}
	}

	for (int i = 0; i < num_threads; i++) {
		double* part_res;
		int result = pthread_join(threads[i], (void**)&part_res);
		if (result != 0) {
			char buf[256];
			strerror_r(result, buf, sizeof(buf));
			fprintf(stderr, "error joining thread: %s\n", buf);
			exit(EXIT_FAILURE);
		}
		res += *part_res;
		free(part_res);
	}

	res *= 4.0;
	printf("pi = %.15f\n", res);
	free(indixes);
	free(threads);
	exit(EXIT_SUCCESS);
}
