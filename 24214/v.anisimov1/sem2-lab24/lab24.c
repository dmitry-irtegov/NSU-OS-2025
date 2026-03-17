#include <pthread.h> 
#include <stdio.h> 
#include <unistd.h>
#include <semaphore.h> 
#include <string.h> 
#define WORKERAMOUNT 3 

typedef struct workerConfig {
    sem_t* unitsDst;
    unsigned int time; 
    int amount; 
} workerConfig; 

void * workerRoutine(void *arg) {
    if (arg == NULL) {
        return (void *) 0; 
    }

    workerConfig *wcfg = (workerConfig *) arg;  

    for (int i = 1; i <= wcfg->amount; ++i) {
        sleep(wcfg->time); 
        sem_post(wcfg->unitsDst);
    }

    return (void *) 0; 
}

int main() {
    int err = 0; 

    int amount = 0;
    printf("Enter products amount: "); 
    scanf("%d", &amount);

    sem_t unitsA, unitsB, unitsC; 
    if (sem_init(&unitsA, 0, 0) != 0) {
        perror("sem_init error"); 
        return 0; 
    }
    if (sem_init(&unitsB, 0, 0) != 0) {
        perror("sem_init error");
        sem_destroy(&unitsA); 
        return 0; 
    }
    if (sem_init(&unitsC, 0, 0) != 0) {
        perror("sem_init error");
        sem_destroy(&unitsA);
        sem_destroy(&unitsB);  
        return 0; 
    }

    sem_t* unitsSrc[3] = {&unitsA, &unitsB, &unitsC};

    workerConfig configA = {unitsSrc[0], 1, amount}; 
    workerConfig configB = {unitsSrc[1], 2, amount}; 
    workerConfig configC = {unitsSrc[2], 3, amount}; 

    workerConfig configPull[WORKERAMOUNT] = {configA, configB, configC}; 
    
    pthread_t workerPull[WORKERAMOUNT] = {}; 

    for (size_t i = 0; i < WORKERAMOUNT; ++i) {
        err = pthread_create(&workerPull[i], NULL, workerRoutine, &configPull[i]);
        if (err != 0) {
            char buff[1024]; 
            strerror_r(err, buff, sizeof(buff)); 
            fprintf(stderr, "ERROR! Creation %s", buff); 
            sem_destroy(&unitsA);
            sem_destroy(&unitsB);
            sem_destroy(&unitsC);
            return 0;
        }
    }

    for (int i = 1; i <= amount; ++i) {
        for (size_t j = 0; j < WORKERAMOUNT; ++j) {
            sem_wait(unitsSrc[j]); 
        }

        printf("The product %d has been produced\n", i);
    }

    for (size_t i = 0; i < WORKERAMOUNT; ++i) {
        err = pthread_join(workerPull[i], NULL);
        if (err != 0) {
            char buff[1024]; 
            strerror_r(err, buff, sizeof(buff)); 
            fprintf(stderr, "ERROR! join %s", buff);
        }
    }

    sem_destroy(&unitsA);
    sem_destroy(&unitsB);
    sem_destroy(&unitsC);
    return 0;
}