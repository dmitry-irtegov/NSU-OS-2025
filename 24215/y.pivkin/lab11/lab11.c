#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t turns[3];
int parent = 0;
int child = 2;
int inited = 0;

void* thread_func() {
    pthread_mutex_lock(&turns[2]);
    inited = 1;
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&turns[(child + 1) % 3]);

        printf("Child thread %d\n", i + 1);

        pthread_mutex_unlock(&turns[child]);
        child = (child + 1) % 3;
    }

    return NULL;
}

int main () {
    pthread_t thread;

    for (int i = 0; i < 3; i++) {
        pthread_mutex_init(&turns[i], NULL);
    }

    if (pthread_create(&thread, NULL, thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

    pthread_mutex_lock(&turns[0]);

    while(!inited){
        //pthread_mutex_unlock(&turns[2]);

        sleep(0.1);
    }

    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&turns[(parent + 1) % 3]);

        printf("Parent thread %d\n", i + 1);

        pthread_mutex_unlock(&turns[parent]);
        parent = (parent + 1) % 3;
    }

    pthread_exit(NULL);
    //exit(0);
}
