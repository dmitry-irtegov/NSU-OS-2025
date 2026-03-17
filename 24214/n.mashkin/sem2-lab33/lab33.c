#include "definitions.h"

Cache *cache;
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
TaskQueue task_queue;
int wakeup_pipe[2];

const char* get_header_value(const char *headers, const char *header_name) {
    char search[64];
    snprintf(search, sizeof(search), "\r\n%s:", header_name);
    
    const char *pos = strcasestr(headers, search);
    if (!pos) {
        snprintf(search, sizeof(search), "%s:", header_name);
        pos = strcasestr(headers, search);
        if (!pos) return NULL;
    }
    
    pos += strlen(search);
    while (*pos == ' ' || *pos == '\t') pos++;
    
    const char *end = pos;
    while (*end && *end != '\r' && *end != '\n') end++;
    
    static char value[256];
    size_t len = end - pos;
    if (len >= sizeof(value)) len = sizeof(value) - 1;
    strncpy(value, pos, len);
    value[len] = '\0';
    return value;
}

ssize_t parse_response_headers(const char *buf, size_t len, size_t *content_length) {
    *content_length = 0;
    
    const char *headers_end = strstr(buf, "\r\n\r\n");
    if (!headers_end) {
        headers_end = strstr(buf, "\n\n");
        if (!headers_end) return -1;
    }
    
    size_t headers_len = headers_end - buf + 4;
    if (headers_len > len) return -1;
    
    const char *cl = get_header_value(buf, "Content-Length");
    if (cl) {
        *content_length = strtoull(cl, NULL, 10);
    }
    
    return headers_len;
}

int client_wants_keepalive(const char *req_buf) {
    const char *conn = get_header_value(req_buf, "Connection");
    const char *proxy_conn = get_header_value(req_buf, "Proxy-Connection");
    
    if (proxy_conn) {
        return strcasecmp(proxy_conn, "keep-alive") == 0;
    }
    if (conn) {
        return strcasecmp(conn, "keep-alive") == 0;
    }
    
    return (strstr(req_buf, "HTTP/1.1") != NULL);
}

int parse_http_request(const char *request, char *host, int host_len, char *path, int path_len) {
    char method[16], url[256];
    
    if (sscanf(request, "%15s %255s", method, url) != 2) {
        return -1;
    }
    
    if (strcmp(method, "GET") != 0) {
        return -2;
    }
    
    if (strncmp(url, "http://", 7) != 0) {
        return -4;
    }

    char *host_start = url + 7;
    char *path_start = strchr(host_start, '/');
    
    if (path_start) {
        int host_length = path_start - host_start;
        if (host_length < host_len) {
            strncpy(host, host_start, host_length);
            host[host_length] = '\0';
            strncpy(path, path_start, path_len);
            path[path_len - 1] = '\0';
        } else {
            return -3;
        }
    } else {
        strncpy(host, host_start, host_len);
        host[host_len - 1] = '\0';
        strncpy(path, "/", path_len);
    }
    
    return 0;
}

int connect_to_server(const char *host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    struct hostent *server = gethostbyname(host);
    if (!server) {
        close(sock);
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);
    
    int result = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (result < 0 && errno != EINPROGRESS) {
        close(sock);
        return -1;
    }
    
    return sock;
}

void send_error_response(int client_fd, int code, const char *message) {
    char response[512];
    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: %lu\r\n"
        "\r\n"
        "<html><body><h1>%d %s</h1></body></html>",
        code, message, strlen(message) + 50, code, message);
    
    send(client_fd, response, len, 0);
}

// Task Queue

void queue_push(ClientTask *task) {
    pthread_mutex_lock(&task_queue.mutex);
    
    if (task_queue.count >= 1024) {
        send_error_response(task->client_fd, 503, "Service Unavailable");
        close(task->client_fd);
        pthread_mutex_unlock(&task_queue.mutex);
        return;
    }
    
    task_queue.tasks[task_queue.tail] = *task;
    task_queue.tail = (task_queue.tail + 1) % 1024;
    task_queue.count++;
    
    pthread_cond_signal(&task_queue.cond);
    pthread_mutex_unlock(&task_queue.mutex);

    write(wakeup_pipe[1], "x", 1);
}

int queue_pop(ClientTask *task, int should_block) {
    pthread_mutex_lock(&task_queue.mutex);
    
    if (should_block) {
        while (task_queue.count == 0 && !task_queue.shutdown) {
            pthread_cond_wait(&task_queue.cond, &task_queue.mutex);
        }
        
        if (task_queue.shutdown && task_queue.count == 0) {
            pthread_mutex_unlock(&task_queue.mutex);
            return 0;
        }
    } else if (task_queue.count == 0) {
        pthread_mutex_unlock(&task_queue.mutex);
        return 0;
    }
    
    *task = task_queue.tasks[task_queue.head];
    task_queue.head = (task_queue.head + 1) % 1024;
    task_queue.count--;
    
    pthread_mutex_unlock(&task_queue.mutex);
    return 1;
}

void queue_shutdown() {
    pthread_mutex_lock(&task_queue.mutex);
    task_queue.shutdown = 1;
    pthread_mutex_unlock(&task_queue.mutex);
}

int find_connection_slot(ThreadState *state) {
    for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
        if (!state->connections[i].active) {
            return i;
        }
    }
    return -1;
}

void init_connection(ThreadState *state, int slot, int client_fd) {
    ThreadConnection *conn = &state->connections[slot];
    
    conn->client_fd = client_fd;
    conn->server_fd = -1;
    conn->state = RECV_REQ;
    conn->active = 1;
    conn->req_len = 0;
    conn->req_sent = 0;
    conn->resp_len = 0;
    conn->resp_sent = 0;
    conn->expected_body_len = 0;
    conn->headers_end_pos = 0;
    conn->server_wants_close = 0;
    if (conn->url) conn->url[0] = '\0';
    if (conn->req_buf) conn->req_buf[0] = '\0';
    
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    if (client_fd > state->max_fd) {
        state->max_fd = client_fd;
    }
}

void close_connection(ThreadState *state, int slot) {
    ThreadConnection *conn = &state->connections[slot];
    
    if (conn->client_fd >= 0) {
        close(conn->client_fd);
        conn->client_fd = -1;
    }
    if (conn->server_fd >= 0) {
        close(conn->server_fd);
        conn->server_fd = -1;
    }
    
    conn->active = 0;
    conn->state = RECV_REQ;
    conn->req_len = 0;
    conn->req_sent = 0;
    conn->resp_len = 0;
    conn->resp_sent = 0;
    conn->expected_body_len = 0;
    conn->headers_end_pos = 0;
    conn->server_wants_close = 0;
    if (conn->url) conn->url[0] = '\0';
    if (conn->req_buf) conn->req_buf[0] = '\0';
    
    state->max_fd = wakeup_pipe[0];
    for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
        if (state->connections[i].active) {
            if (state->connections[i].client_fd > state->max_fd)
                state->max_fd = state->connections[i].client_fd;
            if (state->connections[i].server_fd > state->max_fd)
                state->max_fd = state->connections[i].server_fd;
        }
    }
}

int init_connection_buffers(ThreadConnection *conn) {
    conn->req_buf = malloc(BUFFER_SIZE);
    if (!conn->req_buf) return -1;
    
    conn->url = malloc(512);
    if (!conn->url) {
        free(conn->req_buf);
        return -1;
    }
    
    conn->resp_buf = NULL;
    conn->resp_cap = 0;
    return 0;
}

void free_connection_buffers(ThreadConnection *conn) {
    if (conn->req_buf) {
        free(conn->req_buf);
        conn->req_buf = NULL;
    }
    if (conn->url) {
        free(conn->url);
        conn->url = NULL;
    }
    if (conn->resp_buf) {
        free(conn->resp_buf);
        conn->resp_buf = NULL;
        conn->resp_cap = 0;
    }
}

int init_thread_state(ThreadState *state) {
    memset(state, 0, sizeof(ThreadState));
    
    state->connections = calloc(MAX_CONNECTIONS_PER_THREAD, sizeof(ThreadConnection));
    if (!state->connections) return -1;
    
    for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
        state->connections[i].client_fd = -1;
        state->connections[i].server_fd = -1;
        state->connections[i].active = 0;
        if (init_connection_buffers(&state->connections[i]) < 0) {
            for (int j = 0; j < i; j++) {
                free_connection_buffers(&state->connections[j]);
            }
            free(state->connections);
            state->connections = NULL;
            return -1;
        }
    }
    
    state->max_fd = 0;
    state->connection_count = 0;
    return 0;
}

void free_thread_state(ThreadState *state) {
    if (!state->connections) return;
    
    for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
        free_connection_buffers(&state->connections[i]);
    }
    
    free(state->connections);
    state->connections = NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <port> [thread_count]\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    int thread_count = (argc == 3) ? atoi(argv[2]) : DEFAULT_THREAD_COUNT;
    
    printf("Proxy started on port %d with %d worker threads\n", port, thread_count);
    
    cache = init_cache(100);
    task_queue = (TaskQueue) {
        .head = 0,
        .tail = 0,
        .count = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER,
        .shutdown = 0
    };
    
    if (pipe(wakeup_pipe) < 0) {
        perror("pipe");
        return 1;
    }
    
    int flags;
    flags = fcntl(wakeup_pipe[0], F_GETFL, 0);
    fcntl(wakeup_pipe[0], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(wakeup_pipe[1], F_GETFL, 0);
    fcntl(wakeup_pipe[1], F_SETFL, flags | O_NONBLOCK);
        
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }
    
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    
    if (listen(listen_fd, 1024) < 0) {
        perror("listen");
        return 1;
    }
    
    pthread_t threads[thread_count];
    for (int i = 0; i < thread_count; i++) {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;

        if (pthread_create(&threads[i], NULL, worker_thread, thread_id) != 0) {
            perror("pthread_create");
            free(thread_id);
            continue;
        }
    }
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }
        
        ClientTask task;
        task.client_fd = client_fd;
        task.client_addr = client_addr;
        queue_push(&task);
    }
    
    queue_shutdown();
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    close(listen_fd);
    free_cache(cache);
    pthread_mutex_destroy(&cache_mutex);
    pthread_mutex_destroy(&task_queue.mutex);
    
    return 0;
}
