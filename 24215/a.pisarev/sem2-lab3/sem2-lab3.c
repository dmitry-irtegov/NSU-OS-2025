#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define THREADS_AMOUNT 4

typedef struct ThreadData{
	const char** strings;
	int start;
	int end;
} ThreadData;

void *thread_start(void *arg){
	ThreadData *data=(ThreadData*) arg;
	for(int i=data->start;i<data->end;i++){
		puts(data->strings[i]);
	}
	return NULL;
}

void before_exit(pthread_t *threads, int i){
	for(int j=0;j<i;j++){
		pthread_join(threads[j],NULL);
	}
}

int main(){
	const char *myString[] = {"1","22","333","4444","5555","666666666","777"};

	pthread_t threads[THREADS_AMOUNT];
	int startID=0;
	int endID = 0;
	int amountOfElements = sizeof(myString)/sizeof(myString[0]);

	int elementsPerThread = amountOfElements/THREADS_AMOUNT;
	int remainder = amountOfElements%THREADS_AMOUNT; //amount of threads with extra element

	
	ThreadData threadData[THREADS_AMOUNT];

	for(int i = 0;i<THREADS_AMOUNT;i++){ 
		startID=endID;
		if (i<remainder){ //remainder < THREADS_AMOUNT by definition
			endID+= elementsPerThread+1;
		}
		else {
			endID+=elementsPerThread;
		}
		threadData[i].strings=myString;
		threadData[i].start=startID;
		threadData[i].end = endID;
		if (pthread_create(&threads[i],NULL,thread_start,&threadData[i])!=0)
		{
			before_exit(threads,i);
			perror("Creating thread");
			exit(EXIT_FAILURE);
		}
	}
	for(int i = 0; i<THREADS_AMOUNT;i++){
		pthread_join(threads[i],NULL);
	}
	return 0;
}
