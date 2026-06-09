#ifndef POOL_H
#define POOL_H

#include <pthread.h>

#include "cache.h"

#define QUEUE_MAX 256

typedef struct {
    pthread_t thread;
    int id;
    int wakeup[2];
    int queue[QUEUE_MAX];
    int queue_count;
    int stop;
    pthread_mutex_t lock;
    cache_t* cache;
} worker_t;

typedef struct {
    worker_t* workers;
    int num_workers;
    int next;
} pool_t;

int pool_start(pool_t* pool, int num_workers, cache_t* cache);
void pool_dispatch(pool_t* pool, int client_fd);
void pool_stop(pool_t* pool);

#endif
