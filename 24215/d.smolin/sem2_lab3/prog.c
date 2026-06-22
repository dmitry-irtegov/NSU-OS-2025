#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N_THREADS 4

static const char *lines[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
#define N_LINES (sizeof(lines) / sizeof(lines[0]))

typedef struct {
    int from, to;
} Slice;

void* worker(void* p) {
    Slice* slice = (Slice*)p;
    for (int i = slice->from; i < slice->to; ++i) {
        printf("Поток %ld: %s\n", pthread_self(), lines[i]);
    }
    return NULL;
}

int main() {
    int total = N_LINES;
    int base = total / N_THREADS;
    int extra = total % N_THREADS;
    
    pthread_t tids[N_THREADS];
    Slice slices[N_THREADS];
    
    int pos = 0;
    int active = 0;
    
    printf("%d строк \n %d потоков\n", total, N_THREADS);
    
    for (int t = 0; t < N_THREADS; ++t) {
        int chunk = base + (t < extra ? 1 : 0);
        
        if (pos < total) {
            slices[t] = (Slice){pos, pos + chunk};
            printf("Поток %d: строки %d-%d\n", 
                   t, pos, pos + chunk - 1);
            
            if (pthread_create(&tids[t], NULL, worker, &slices[t]) != 0) {
                perror("pthread_create failed");
                for (int j = 0; j < t; ++j) pthread_join(tids[j], NULL);
                return 1;
            }
            active++;
        }
        pos += chunk;
    }
        
    for (int t = 0; t < active; ++t) {
        pthread_join(tids[t], NULL);
    }
    
    return 0;
}
