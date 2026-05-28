#define _GNU_SOURCE
#include "common.h"
#include "event_loop.h"
#include "cache.h"
#include "utils.h"
#include <arpa/inet.h> 

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
        fprintf(stderr, "Invalid bind address: %s\n", bind_addr);
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
    
    mkdir(LOG_DIR, 0755);
    
    for (int i = 0; i < MAX_FDS; i++) {
        clients[i].fd = -1;
        clients[i].cache_idx = -1;
        clients[i].upstream_idx = -1;
    }
    
    fprintf(stderr, "[LOG] Proxy starting on %s:%d\n", bind_addr, port);
    
    int listen_fd = create_listener(port, bind_addr);
    if (listen_fd < 0) {
        fprintf(stderr, "[LOG] Failed to initialize listener\n");
        return 1;
    }
    
    event_loop_init(listen_fd);
    event_loop_run();
    
    event_loop_cleanup();
    safe_close(listen_fd);
    cache_cleanup_all();
    
    fprintf(stderr, "[LOG] Proxy stopped\n");
    return 0;
}