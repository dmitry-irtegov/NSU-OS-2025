#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SLEEP_COEFFICIENT 10000
#define MAX_BUF_LENGTH 1000
#define MAX_STRINGS_AMOUNT 100

pthread_mutex_t printer_lock = PTHREAD_MUTEX_INITIALIZER;

void *sleepSort(void *arg){
	unsigned int stringLength = strlen((char*)arg);
	unsigned int sleepTime = SLEEP_COEFFICIENT * stringLength;
	usleep(sleepTime);
	pthread_mutex_lock(&printer_lock);
	printf("%s\n",(char*)arg);
	pthread_mutex_unlock(&printer_lock);
	return NULL;
}

void before_exit(pthread_t *threads, int i){
	for(int j=0;j<i;j++){
		pthread_join(threads[j],NULL);
	}
}


int main(int argc, char* argv[]){
	pthread_t threads[MAX_STRINGS_AMOUNT];
	char* strings[MAX_STRINGS_AMOUNT];
	int count = 0;
	char* buf = malloc(sizeof(char) * MAX_BUF_LENGTH);
	size_t size = MAX_BUF_LENGTH;
	while (count <MAX_STRINGS_AMOUNT && fgets(buf, size, stdin)!=NULL){
		int stringLength = strlen(buf);
		if (stringLength>0 && buf[stringLength-1]=='\n'){
			buf[stringLength-1] = '\0';
			stringLength--;
		}
		int stringSize = stringLength +1;//+1 for '\0'
		strings[count] = (char*)malloc(sizeof(char)*stringSize);
		if (strings[count] == NULL){
			perror("malloc");
			for(int j=0;j<count;j++){
				free(strings[j]);
			}
			free(buf);
			exit(1);
		}
		strcpy(strings[count], buf);
		count++;
	}
	for(int i=0;i<count;i++){
		if (pthread_create(&threads[i],NULL,sleepSort,strings[i])!=0)
		{
			perror("Creating thread");
			before_exit(threads,i);
			for(int j=0;j<count;j++){
				free(strings[j]);
			}
			free(buf);
			exit(1);
		}
	}
	
	for(int i=0;i<count;i++){
		if (pthread_join(threads[i], NULL)!=0)
		{
			perror("Joining thread");
			for(int j=0;j<count;j++){
				free(strings[j]);
			}
			free(buf);
			exit(1);
		}
	}
	free(buf);
	for(int j=0;j<count;j++){
		free(strings[j]);
	}
	return 0;
}
