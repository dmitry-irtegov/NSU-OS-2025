#include <stdio.h>
#include <pthread.h>
#include <string.h>

typedef struct {
    char **strings;
    int count;
} ThreadData;

void *thread_routine(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int i = 0; i < data->count; i++) {
        printf("%s\n", data->strings[i]);
    }
    return NULL;
}

int main() {
    pthread_t threads[4];
    ThreadData thread_data[4];
    
    char *thread1_strings[] = {"Thread 1", "Thread 1", "Thread 1"};
    thread_data[0].strings = thread1_strings;
    thread_data[0].count = 3;
    
    char *thread2_strings[] = {"Thread 2", "Thread 2", "Thread 2", "Thread 2"};
    thread_data[1].strings = thread2_strings;
    thread_data[1].count = 4;
    
    char *thread3_strings[] = {"Thread 3", "Thread 3", "Thread 3", "Thread 3", "Thread 3"};
    thread_data[2].strings = thread3_strings;
    thread_data[2].count = 5;
    
    char *thread4_strings[] = {"Thread 4", "Thread 4", "Thread 4", "Thread 4", "Thread 4", "Thread 4"};
    thread_data[3].strings = thread4_strings;
    thread_data[3].count = 6;
    
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, thread_routine, &thread_data[i]);
    }
    
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
