#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>

#define NO_INTERRUPT 0
#define INTERRUPT_RECEIVED 1


typedef struct ThreadData {
	int ID;
	int n; //skip n pairs
	double returnValue;
} ThreadData;

pthread_mutex_t lock= PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t checkBarrier;
volatile sig_atomic_t stopComputing = NO_INTERRUPT;
volatile long lastIteration;
void handle_sigint(int sig){
	stopComputing = INTERRUPT_RECEIVED;
}

void *thread_start(void *arg){
	ThreadData* data = (ThreadData*) arg;
	long id = data->ID;
	long n = data->n;
	long iterations=0;
	double returnValue = 0;
	long initLastIterationValue = 0;
	for(long i = id; ; i += n){
		iterations++;
		if (iterations % 100==0 && stopComputing==INTERRUPT_RECEIVED){
				atomic_compare_exchange_strong(&lastIteration, &initLastIterationValue, iterations);
				if(iterations>=lastIteration){
					pthread_barrier_wait(&checkBarrier);
					break;
				}
		}
		returnValue += 1.0/(i*4.0 + 1.0);
		returnValue -= 1.0/(i*4.0 + 3.0);
	}
	data->returnValue=returnValue;
	return NULL;
}

void before_exit(pthread_t *threads, int i){
	for(int j = 0;j < i;j++){
		pthread_join(threads[j],NULL);
	}
}

int main(int argc, char *argv[]){
	if (argc < 2) {
		fprintf(stderr, "Amount of threads is missed!");
		return 1;
	}
	int threads_amount = atoi(argv[1]);
	if (threads_amount < 1) {
		fprintf(stderr,"Wrong amount of threads!");
		return 1;
	}

	struct sigaction sa;
	sa.sa_handler = handle_sigint;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGINT,&sa,NULL) == -1){
		perror("sigaction");
		return 1;
	}
	pthread_barrier_init(&checkBarrier,NULL,threads_amount);
	lastIteration = 0;
	pthread_t *threads = (pthread_t*) malloc(sizeof(pthread_t)*threads_amount);
	ThreadData *threadsData = (ThreadData*) malloc(sizeof(ThreadData)*threads_amount);
	
	for(int i = 0; i < threads_amount; i++) {
		threadsData[i].ID = i;
		threadsData[i].n = threads_amount;
		threadsData[i].returnValue = 0;
		if (pthread_create(&threads[i],NULL,thread_start,&threadsData[i]) != 0) {
			perror("Creating thread");
			before_exit(threads,i);
			free(threadsData);
			free(threads);
			exit(1);
		}
	}
	double result = 0;
	for(int i = 0; i < threads_amount; i++){
		pthread_join(threads[i],NULL);
		result+=threadsData[i].returnValue*4.0;
	}
	printf("\nresult: %.10f\n",result);
	free(threadsData);
	free(threads);
	pthread_barrier_destroy(&checkBarrier);
	return 0;
}
