#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

void* thread_func(void* param) {
    for (int i = 0; i< 10; i++) {
        printf("good\n");
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    pthread_t thread;

    int result = pthread_create(&thread, NULL, thread_func, NULL);
    if (result != 0){
        char buf[256];
        strerror_r(result, buf, sizeof(buf));
        fprintf(stderr, "error creating thread: %s\n", buf);
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < 10; i++) {
        printf("excellent\n");
    }

    pthread_exit(NULL);

}