#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


void cleanUpFunc(void *arg){
	printf("Clean up function called!");
}

void *printText(void *arg){
	pthread_cleanup_push(cleanUpFunc, NULL);
	while (1){
		printf("I am alive!\n");
		sleep(1);
	}
	pthread_cleanup_pop(1);
	return NULL;
}
int main(){
	pthread_t thread;
	if (pthread_create(&thread,NULL,printText,NULL)!=0)
	{
		perror("Creating thread");
		exit(1);
	}
	sleep(2);
	if (pthread_cancel(thread)!=0)
	{
		perror("Cancelling thread");
		exit(1);
	}
	if (pthread_join(thread, NULL)!=0)
	{
		perror("Joining thread");
		exit(1);
	}
	return 0;
}
