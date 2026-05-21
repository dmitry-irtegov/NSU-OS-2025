#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define MAX_COUNT_ID 200000000

typedef struct ThreadData {
	int ID;
	int n; //skip n pairs
	double returnValue;
} ThreadData;


void *thread_start(void *arg){
	ThreadData* data = (ThreadData*) arg;
	int id = data->ID;
	int n = data->n;
	double returnValue = 0;
	
	for(int i = id; i<MAX_COUNT_ID ; i += n){
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
		fprintf(stderr, "Amount of threads is missed!\n");
		return 1;
	}
	int threads_amount = atoi(argv[1]);
	if (threads_amount < 1) {
		fprintf(stderr,"Wrong amount of threads!\n");
		return 1;
	}
	struct timespec start,end;
	clock_gettime(CLOCK_MONOTONIC, &start); 
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
	for(int i = 0;i < threads_amount;i++){
		pthread_join(threads[i],NULL);
		result+=threadsData[i].returnValue;
	}
	result*= 4.0;
	clock_gettime(CLOCK_MONOTONIC, &end); 
	printf("result:%.10f\n",result);
	double res;
	if (end.tv_nsec < start.tv_nsec) {
        res= (end.tv_sec - start.tv_sec - 1) + 
               (end.tv_nsec + 1000000000L - start.tv_nsec) / 1e9;
    } else {
        res= (end.tv_sec - start.tv_sec) + 
               (end.tv_nsec - start.tv_nsec) / 1e9;
    }
	printf("time: %lf\n",res);
	free(threadsData);
	free(threads);
	return 0;
}
