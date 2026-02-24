#include <stdio.h> 
#include <inttypes.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h> 
#define MAX_THREAD_AMOUNT 6 

uint32_t toy_hash(uint32_t x, uint32_t salt) {
    uint32_t h = x ^ salt;
    h *= 0x7feb352d;
    h ^= h >> 15;
    h *= 0x846ca68b;
    h ^= h >> 16;
    return h;
}

typedef struct range {
    uint32_t Start; 
    uint32_t End; 
} range;

typedef struct result {
    uint32_t salt; 
    uint32_t hash;
    int success; 
} result;

typedef struct task {
    range Range; 
    uint32_t x; 
    int n;
    result Res;
    int *flag; 
    pthread_mutex_t *mtx; 
} task;  

void fillTasks(task Tasks[], 
                int threadsAmount, 
                uint32_t x, 
                int n,
                pthread_mutex_t *mtx, 
                int *flag) {
    uint64_t totalSpace = (uint64_t)UINT32_MAX + 1;
    uint64_t chunkSize = totalSpace / threadsAmount;
    uint64_t remainder = totalSpace % threadsAmount;
    uint32_t currentStart = 0;
    for (int i = 0; i < threadsAmount; ++i) {
        Tasks[i].x = x;
        Tasks[i].n = n;
        Tasks[i].Res.success = 0; 
        uint64_t currentChunk = chunkSize + ((uint64_t) i < remainder ? 1 : 0);
        Tasks[i].Range.Start = currentStart; 
        Tasks[i].Range.End = (uint32_t) (currentStart + currentChunk - 1);
        Tasks[i].mtx = mtx; 
        Tasks[i].flag = flag; 
        currentStart += (uint32_t) currentChunk;
    }
}

void *subroutine(void *arg) {
    task *Task = (task *) arg;
    pthread_mutex_lock(Task->mtx);
        if (*(Task->flag)) {
            pthread_mutex_unlock(Task->mtx);
            return NULL; 
        }
    pthread_mutex_unlock(Task->mtx);
    for (uint64_t i = Task->Range.Start; i <= Task->Range.End; ++i) {
        uint32_t hash = toy_hash(Task->x, (uint32_t) i); 
        uint32_t checker = 1;
        int nonZeroBit = 0;  
        for (int j = 0; j < Task->n; ++j) {
            if ((hash & (checker << j)) != 0) {
                nonZeroBit = 1;
                break; 
            }
        }
        if (!nonZeroBit) {
            pthread_mutex_lock(Task->mtx);
            if (!(*(Task->flag))) {
                Task->Res.hash = hash;
                Task->Res.salt = (uint32_t) i; 
                Task->Res.success = 1; 
                *(Task->flag) = 1;
            }
            pthread_mutex_unlock(Task->mtx);
            return NULL; 
        }
    }
    return NULL; 
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Uncorrect usage! Enter the thread amount\n");
        return 0;  
    }

    int threadAmount; 
    if (sscanf(argv[1], "%d", &threadAmount) != 1) {
        perror("[ERROR!] Cannot read the thread amount!");
        return 0;
    }
    
    if (threadAmount <= 0) {
        threadAmount = 1; 
    }

    if (threadAmount > MAX_THREAD_AMOUNT) {
        threadAmount = MAX_THREAD_AMOUNT; 
    }

    uint32_t x; 
    int n;
    printf("Enter x and n: ");
    if (scanf("%" SCNu32 "%d", &x, &n) != 2) {
        perror("[ERROR] Cannot read x and n");
        return 0; 
    }

    task Tasks[threadAmount]; 
    pthread_t threadsPull[threadAmount]; 
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; 
    int flag = 0; 

    fillTasks(Tasks, threadAmount, x, n, &mtx, &flag); 

    int err = 0; 

    for (int i = 0; i < threadAmount; ++i) {
        err = pthread_create(&(threadsPull[i]),
                            NULL, subroutine, 
                            &(Tasks[i])); 
        if (err != 0) {
            char buff[1024]; 
            strerror_r(err, buff, sizeof(buff)); 
            fprintf(stderr, "ERROR! Create %s", buff); 
            return 0;
        }
    }

    for (int i = 0; i < threadAmount; ++i) {
        err = pthread_join(threadsPull[i], NULL); 
        if (err != 0) {
            char buff[1024]; 
            strerror_r(err, buff, sizeof(buff)); 
            fprintf(stderr, "ERROR! Join %s", buff); 
            return 0; 
        }
        if (Tasks[i].Res.success) {
            printf("SALT: %" PRIu32 " | HASH: %" PRIu32 "\n", 
                Tasks[i].Res.salt, 
                Tasks[i].Res.hash);
        }
    }
    printf("The search has been finished\n");
    err = pthread_mutex_destroy(&mtx); 
    if (err != 0) {
        char buff[1024]; 
        strerror_r(err, buff, sizeof(buff)); 
        fprintf(stderr, "ERROR! Destroy %s", buff); 
        return 0;
    }
    return 0; 
}