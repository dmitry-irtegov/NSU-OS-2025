#ifndef DEFINITIONS
#define DEFINITIONS

#include "cache.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/select.h>

#define BUFFER_SIZE 8192
#define MAX_CONNECTIONS_PER_THREAD 256
#define DEFAULT_THREAD_COUNT 4

#define RECV_REQ 0
#define CONNECTING 1
#define SEND_REQ 2
#define RECV_RESP 3
#define SEND_RESP 4

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} ClientTask;

typedef struct {
    ClientTask tasks[1024];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int shutdown;
} TaskQueue;

typedef struct {
    int client_fd;
    int server_fd;
    char req_buf[BUFFER_SIZE];
    size_t req_len;
    size_t req_sent;
    char *resp_buf;
    size_t resp_cap;
    size_t resp_len;
    size_t resp_sent;
    size_t expected_body_len;
    size_t headers_end_pos;
    int server_wants_close;
    char url[512];
    int state;
    int active;
} ThreadConnection;

typedef struct {
    ThreadConnection connections[MAX_CONNECTIONS_PER_THREAD];
    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;
    int max_fd;
    int connection_count;
} ThreadState;


#endif // DEFINITIONS
