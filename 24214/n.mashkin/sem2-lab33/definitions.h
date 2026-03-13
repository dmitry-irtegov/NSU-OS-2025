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
    char *req_buf;
    size_t req_len;
    size_t req_sent;
    char *resp_buf;
    size_t resp_cap;
    size_t resp_len;
    size_t resp_sent;
    size_t expected_body_len;
    size_t headers_end_pos;
    int server_wants_close;
    char *url;
    int state;
    int active;
} ThreadConnection;

typedef struct {
    ThreadConnection *connections;
    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;
    int max_fd;
    int connection_count;
} ThreadState;

const char* get_header_value(const char *headers, const char *header_name);
ssize_t parse_response_headers(const char *buf, size_t len, size_t *content_length);
int client_wants_keepalive(const char *req_buf);
int parse_http_request(const char *request, char *host, int host_len, char *path, int path_len);
int connect_to_server(const char *host, int port);
void send_error_response(int client_fd, int code, const char *message);
int queue_pop(ClientTask *task, int should_block);
int find_connection_slot(ThreadState *state);
void init_connection(ThreadState *state, int slot, int client_fd);
void close_connection(ThreadState *state, int slot);
int init_connection_buffers(ThreadConnection *conn);
void free_connection_buffers(ThreadConnection *conn);
int init_thread_state(ThreadState *state);
void free_thread_state(ThreadState *state);
void* worker_thread(void *arg);

#endif // DEFINITIONS
