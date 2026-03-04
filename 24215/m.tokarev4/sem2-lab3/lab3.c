#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

pthread_t thread[4];

typedef struct {
    int id;
    char* (*string)[4];
} ThreadInfo;

void* printlines(void* arg) {
    ThreadInfo* strct = (ThreadInfo*)arg;

    int ind = strct->id;

    for (int j = 0; j < 4; j++) {
        printf("%s\n", strct->string[ind][j]);
    }

    return NULL;
}

int main() {

    ThreadInfo* args[4];

    char* info[4][4] = { { "Thread 0: one","Thread 0: two", "Thread 0: three", "Thread 0: four" },
            { "Thread 1: one","Thread 1: two", "Thread 1: three", "Thread 1: four" },
            { "Thread 2: one","Thread 2: two", "Thread 2: three", "Thread 2: four" },
            { "Thread 3: one","Thread 3: two", "Thread 3: three", "Thread 3: four" } };

    for (int i = 0; i < 4; i++) {
        args[i] = (ThreadInfo*)malloc(sizeof(ThreadInfo));
        args[i]->id = i;
        args[i]->string = info;
        pthread_create(&thread[i], NULL, printlines, (void*)args[i]);

    };

    for(int j = 0; j < 4; j++){
	pthread_join(thread[j], NULL);
	free(args[j]);
    }
    return 0;
    
}

