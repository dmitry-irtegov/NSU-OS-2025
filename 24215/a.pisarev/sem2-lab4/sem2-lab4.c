#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


void *printText(void *arg){
	while (1){
		printf("I am alive!\n");
		sleep(1);
	}
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
	return 0;
}
