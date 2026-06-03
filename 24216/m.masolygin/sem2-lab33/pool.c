#include "pool.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "conn.h"

void* worker_loop(void* arg) {
    worker_t* worker = (worker_t*)arg;
    conn_t* conns[FD_SETSIZE];
    int conn_count = 0;
    int stop = 0;

    while (!stop) {
        fd_set read_fds;
        fd_set write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(worker->wakeup[0], &read_fds);
        int max_fd = worker->wakeup[0];

        for (int i = 0; i < conn_count; i++) {
            int fd = conn_active_fd(conns[i]);
            if (fd < 0) {
                continue;
            }
            if (conn_wants_write(conns[i])) {
                FD_SET(fd, &write_fds);
            } else {
                FD_SET(fd, &read_fds);
            }
            if (fd > max_fd) {
                max_fd = fd;
            }
        }

        if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        if (FD_ISSET(worker->wakeup[0], &read_fds)) {
            char drain[256];
            ssize_t got = read(worker->wakeup[0], drain, sizeof(drain));
            (void)got;
            pthread_mutex_lock(&worker->lock);
            stop = worker->stop;
            while (worker->queue_count > 0) {
                int fd = worker->queue[--worker->queue_count];
                conn_t* conn = conn_create(fd, worker->cache);
                if (conn != NULL) {
                    conns[conn_count++] = conn;
                } else {
                    shutdown(fd, SHUT_RDWR);
                    close(fd);
                }
            }
            pthread_mutex_unlock(&worker->lock);
        }

        for (int i = 0; i < conn_count;) {
            int fd = conn_active_fd(conns[i]);
            int ready = 0;
            if (fd >= 0) {
                ready = conn_wants_write(conns[i]) ? FD_ISSET(fd, &write_fds)
                                                   : FD_ISSET(fd, &read_fds);
            }
            if (ready) {
                conn_advance(conns[i]);
            }
            if (conns[i]->state == CONN_DONE) {
                conn_destroy(conns[i]);
                conns[i] = conns[--conn_count];
                continue;
            }
            i++;
        }
    }

    for (int i = 0; i < conn_count; i++) {
        conn_destroy(conns[i]);
    }

    return NULL;
}

int pool_start(pool_t* pool, int num_workers, cache_t* cache) {
    pool->workers = malloc(num_workers * sizeof(worker_t));
    if (pool->workers == NULL) {
        return 1;
    }
    pool->num_workers = num_workers;
    pool->next = 0;

    for (int i = 0; i < num_workers; i++) {
        worker_t* worker = &pool->workers[i];
        worker->id = i;
        worker->cache = cache;
        worker->queue_count = 0;
        worker->stop = 0;
        pthread_mutex_init(&worker->lock, NULL);
        if (pipe(worker->wakeup) != 0) {
            perror("pipe");
            return 1;
        }
        if (pthread_create(&worker->thread, NULL, worker_loop, worker) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    return 0;
}

void reject_client(int client_fd) {
    char* message =
        "HTTP/1.0 503 Service Unavailable\r\nConnection: close\r\n"
        "\r\nService Unavailable\n";
    send(client_fd, message, strlen(message), 0);
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
}

void pool_dispatch(pool_t* pool, int client_fd) {
    for (int i = 0; i < pool->num_workers; i++) {
        worker_t* worker = &pool->workers[pool->next];
        pool->next = (pool->next + 1) % pool->num_workers;

        pthread_mutex_lock(&worker->lock);
        if (worker->queue_count < QUEUE_MAX) {
            worker->queue[worker->queue_count++] = client_fd;
            pthread_mutex_unlock(&worker->lock);
            char byte = 1;
            ssize_t put = write(worker->wakeup[1], &byte, 1);
            (void)put;
            return;
        }
        pthread_mutex_unlock(&worker->lock);
    }

    reject_client(client_fd);
}

void pool_stop(pool_t* pool) {
    for (int i = 0; i < pool->num_workers; i++) {
        worker_t* worker = &pool->workers[i];
        pthread_mutex_lock(&worker->lock);
        worker->stop = 1;
        pthread_mutex_unlock(&worker->lock);
        char byte = 1;
        ssize_t put = write(worker->wakeup[1], &byte, 1);
        (void)put;
    }

    for (int i = 0; i < pool->num_workers; i++) {
        pthread_join(pool->workers[i].thread, NULL);
        close(pool->workers[i].wakeup[0]);
        close(pool->workers[i].wakeup[1]);
        pthread_mutex_destroy(&pool->workers[i].lock);
    }

    free(pool->workers);
    pool->workers = NULL;
}
