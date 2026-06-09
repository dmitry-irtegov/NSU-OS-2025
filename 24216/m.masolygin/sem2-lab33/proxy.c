#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cache.h"
#include "listener.h"
#include "pool.h"

volatile sig_atomic_t server_running = 1;

#define CACHE_CAPACITY 10

int g_listen_fd = -1;

void on_signal(int sig) {
    (void)sig;
    server_running = 0;
    if (g_listen_fd >= 0) {
        shutdown(g_listen_fd, SHUT_RDWR);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Use: %s <port> <pool_size>\n", argv[0]);
        return 1;
    }

    int pool_size = atoi(argv[2]);
    if (pool_size <= 0) {
        fprintf(stderr, "Error: pool size must be positive\n");
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    cache_t cache;
    if (cache_init(&cache, CACHE_CAPACITY) != 0) {
        fprintf(stderr, "Error: cannot init cache\n");
        return 1;
    }

    sigset_t mask;
    sigset_t oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &mask, &oldmask);

    pool_t pool;
    if (pool_start(&pool, pool_size, &cache) != 0) {
        fprintf(stderr, "Error: cannot start thread pool\n");
        return 1;
    }

    pthread_sigmask(SIG_SETMASK, &oldmask, NULL);

    g_listen_fd = listener_open(argv[1]);
    if (g_listen_fd < 0) {
        return 1;
    }

    fprintf(stderr, "proxy listening on port %s with %d workers\n", argv[1],
            pool_size);

    while (server_running) {
        int client = listener_accept(g_listen_fd);
        if (client < 0) {
            continue;
        }
        pool_dispatch(&pool, client);
    }

    close(g_listen_fd);
    pool_stop(&pool);
    cache_destroy(&cache);
    return 0;
}
