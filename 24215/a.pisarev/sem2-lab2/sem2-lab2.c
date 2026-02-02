#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *printTenRows(void *arg){
	for(int i = 0; i < 10; i++){
		printf("Current number is:%d (%d)\n", i, *(int*)arg);
	}
	return NULL;
}


int main(){
	pthread_t thread;
	int thread_num = 1;
	if (pthread_create(&thread,NULL,printTenRows,&thread_num) != 0)
	{
		perror("Creating thread");
		exit(1);
	}
	if (pthread_join(thread, NULL)!=0)
	{
		perror("Joining thread");
		exit(1);
	}
	int main_thread_num=2;
	printTenRows(&main_thread_num);
	return 0;
}
