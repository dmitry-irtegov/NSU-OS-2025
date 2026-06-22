#define _GNU_SOURCE
#include "common.h"
#include "client.h"
#include "cache.h"
#include "utils.h"
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>

volatile sig_atomic_t running = 1;
int active_threads = 0;
pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t threads_cond = PTHREAD_COND_INITIALIZER;

static atomic_ulong req_counter = 0;

static void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

static int create_listener(int port, const char *bind_addr) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, bind_addr, &addr.sin_addr) != 1) {
        log_msg(stderr, "[MAIN] invalid bind address: %s\n", bind_addr);
        safe_close(fd);
        return -1;
    }
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        safe_close(fd);
        return -1;
    }

    if (listen(fd, 128) < 0) {
        perror("listen");
        safe_close(fd);
        return -1;
    }

    return fd;
}

int main(int argc, char **argv) {
    int port = 8080;
    const char *bind_addr = "127.0.0.1";

    if (argc > 1) port = atoi(argv[1]);
    if (argc > 2) bind_addr = argv[2];

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    mkdir(LOG_DIR, 0755); 
    log_msg(stderr, "[MAIN] starting proxy on %s:%d\n", bind_addr, port);

    int listen_fd = create_listener(port, bind_addr);
    if (listen_fd < 0) {
        log_msg(stderr, "[MAIN] failed to initialize listener\n");
        return 1;
    }
    log_msg(stderr, "[MAIN] listener ready fd=%d\n", listen_fd);

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }
        unsigned long req_id = atomic_fetch_add(&req_counter, 1);
        char log_path[256];
        snprintf(log_path, sizeof(log_path), "%s/req_%lu.log", LOG_DIR, req_id);
        FILE *log = fopen(log_path, "w");
        if (!log) log = stderr;

        struct timeval tv = {5, 0}; 
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        log_msg(log, "[MAIN] accepted connection fd=%d\n", client_fd);

        pthread_mutex_lock(&threads_mutex);
        while (active_threads >= MAX_THREADS && running) {
            pthread_cond_wait(&threads_cond, &threads_mutex);
        }

        if (!running) {
            pthread_mutex_unlock(&threads_mutex);
            safe_close(client_fd);
            break;
        }

        active_threads++;
        pthread_mutex_unlock(&threads_mutex);

        Client *client = malloc(sizeof(Client));
        if (!client) {
            log_msg(log, "[MAIN] malloc failed for client struct\n");
            safe_close(client_fd);
            pthread_mutex_lock(&threads_mutex);
            active_threads--;
            pthread_mutex_unlock(&threads_mutex);
            continue;
        }

        memset(client, 0, sizeof(Client));
        client->fd = client_fd; 
        client->log = log;
        client->req_id = req_id;

        pthread_t thread;
        if (pthread_create(&thread, NULL, client_thread, client) != 0) {
            perror("pthread_create");
            safe_close(client_fd);
            free(client);
            pthread_mutex_lock(&threads_mutex);
            active_threads--;
            pthread_mutex_unlock(&threads_mutex);
            continue;
        }

        pthread_detach(thread);
    }

    log_msg(stderr, "[MAIN] shutting down\n");
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5;
    pthread_mutex_lock(&threads_mutex);
    while (active_threads > 0) {
        if (pthread_cond_timedwait(&threads_cond, &threads_mutex, &ts) == ETIMEDOUT) {
            break;
        }
    }
    pthread_mutex_unlock(&threads_mutex);


    safe_close(listen_fd);
    cache_cleanup_all();
    pthread_mutex_destroy(&threads_mutex);
    pthread_cond_destroy(&threads_cond);
    pthread_mutex_destroy(&cache_list_mutex);

    log_msg(stderr, "[MAIN] proxy stopped\n");
    exit(0);
}