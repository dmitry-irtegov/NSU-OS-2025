#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define PARENT_THREAD 0
#define CHILD_THREAD 1

pthread_mutex_t mutex[3];
int notStart = 1;

void *printTenRows(void *arg) {
  int id = *(int*)arg;
	if (id ==PARENT_THREAD){
    	int parentCurr = 1;
		for (int i = 0; i < 10; i++) {
		    printf("Current number is:%d (%d)\n", i, id);
		    pthread_mutex_unlock(&mutex[(parentCurr+3-1)%3]);
		    parentCurr = (parentCurr+1)%3;
		    pthread_mutex_lock(&mutex[parentCurr]);
		}
	}
	else {
		if (id == CHILD_THREAD) {
			int childCurr = 2;
			pthread_mutex_lock(&mutex[childCurr]);
			notStart = 0;

			for (int i = 0; i < 10; i++) {
				pthread_mutex_lock(&mutex[(childCurr+1)%3]);
				printf("Current number is:%d (%d)\n", i, id);

				pthread_mutex_unlock(&mutex[childCurr]);
				childCurr = (childCurr+1)%3;
			}
		}
	}
  return NULL;
}



int main() {
    pthread_t thread;
    for (int i = 0; i < 3; i++) {
        pthread_mutex_init(&mutex[i], NULL);
    }
    pthread_mutex_lock(&mutex[0]);
    pthread_mutex_lock(&mutex[1]);
    int thread_num = CHILD_THREAD;
    if (pthread_create(&thread, NULL, printTenRows, &thread_num) != 0) {
        fprintf(stderr, "Error, while creating a thread\n");
		    for (int i = 0; i < 3; i++) {
		      pthread_mutex_destroy(&mutex[i]);
		    }
        exit(1);
    }

    while (notStart) {
        usleep(50000);
    }
    int main_thread_num = PARENT_THREAD;
    printTenRows(&main_thread_num);

	  for (int i = 0; i < 3; i++) {
	    pthread_mutex_destroy(&mutex[i]);
	  }
    return 0;
}
