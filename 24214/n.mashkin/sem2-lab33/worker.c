#include "definitions.h"

extern Cache *cache;
extern pthread_mutex_t cache_mutex;
extern TaskQueue task_queue; 
extern int wakeup_pipe[2];

void* worker_thread(void *arg) {
    int thread_id = *(int *)arg;
    free(arg);
    
    printf("[Worker %d] Started\n", thread_id);
    
    ThreadState state;
    if (init_thread_state(&state) < 0) {
        perror("[Worker] init_thread_state");
        return NULL;
    }

    while (1) {
        int has_active = 0;
        for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
            if (state.connections[i].active) {
                has_active = 1;
                break;
            }
        }

        ClientTask task;
        if (queue_pop(&task, has_active == 0)) {
            int slot = find_connection_slot(&state);
            if (slot < 0) {
                printf("[Worker %d] Connection limit reached\n", thread_id);
                send_error_response(task.client_fd, 503, "Service Unavailable");
                close(task.client_fd);
            } else {
                printf("[Worker %d] New client fd=%d\n", thread_id, task.client_fd);
                init_connection(&state, slot, task.client_fd);
            }
        }
        
        if (task_queue.shutdown) {
            int still_active = 0;
            for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
                if (state.connections[i].active) {
                    still_active = 1;
                    break;
                }
            }
            if (!still_active) break;
        }
        
        has_active = 0;
        for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
            if (state.connections[i].active) {
                has_active = 1;
                break;
            }
        }
        if (!has_active) {
            continue;
        }
        
        FD_ZERO(&state.read_fds);
        FD_ZERO(&state.write_fds);
        FD_ZERO(&state.except_fds);
        
        int local_max_fd = 0;
        
        FD_SET(wakeup_pipe[0], &state.read_fds);
        if (wakeup_pipe[0] > local_max_fd) {
            local_max_fd = wakeup_pipe[0];
        }

        for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
            ThreadConnection *conn = &state.connections[i];
            if (!conn->active) continue;
            
            if (conn->client_fd >= 0) {
                FD_SET(conn->client_fd, &state.except_fds);
                if (conn->client_fd > local_max_fd) local_max_fd = conn->client_fd;
            }
            if (conn->server_fd >= 0) {
                FD_SET(conn->server_fd, &state.except_fds);
                if (conn->server_fd > local_max_fd) local_max_fd = conn->server_fd;
            }
            
            switch (conn->state) {
                case RECV_REQ:
                    if (conn->client_fd >= 0) {
                        FD_SET(conn->client_fd, &state.read_fds);
                    }
                    break;
                case CONNECTING:
                case SEND_REQ:
                    if (conn->server_fd >= 0) {
                        FD_SET(conn->server_fd, &state.write_fds);
                    }
                    break;
                case RECV_RESP:
                    if (conn->server_fd >= 0) {
                        FD_SET(conn->server_fd, &state.read_fds);
                    }
                    break;
                case SEND_RESP:
                    if (conn->client_fd >= 0) {
                        FD_SET(conn->client_fd, &state.write_fds);
                    }
                    break;
            }
        }
        
        int activity = select(local_max_fd + 1, &state.read_fds, &state.write_fds, &state.except_fds, NULL);
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("[Worker] select");
            break;
        }

        if (FD_ISSET(wakeup_pipe[0], &state.read_fds)) {
            char buf[64];
            while (read(wakeup_pipe[0], buf, sizeof(buf)) > 0);
        }
        
        for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
            ThreadConnection *conn = &state.connections[i];
            if (!conn->active) continue;
            
            if ((conn->client_fd >= 0 && FD_ISSET(conn->client_fd, &state.except_fds)) ||
                (conn->server_fd >= 0 && FD_ISSET(conn->server_fd, &state.except_fds))) {
                close_connection(&state, i);
                continue;
            }
            
            switch (conn->state) {
                case RECV_REQ:
                    if (conn->client_fd >= 0 && FD_ISSET(conn->client_fd, &state.read_fds)) {
                        ssize_t n = recv(conn->client_fd, conn->req_buf + conn->req_len, 
                                        BUFFER_SIZE - conn->req_len - 1, 0);
                        
                        if (n == 0) {
                            printf("[Worker %d] Client fd=%d closed\n", thread_id, conn->client_fd);
                            close_connection(&state, i);
                            continue;
                        }
                        if (n < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                            perror("[Worker] recv client");
                            close_connection(&state, i);
                            continue;
                        }
                        
                        conn->req_len += n;
                        conn->req_buf[conn->req_len] = '\0';
                        
                        if (strstr(conn->req_buf, "\r\n\r\n") || strstr(conn->req_buf, "\n\n")) {
                            char host[128] = {0};
                            char path[256] = {0};
                            
                            if (parse_http_request(conn->req_buf, host, sizeof(host), path, sizeof(path)) != 0) {
                                send_error_response(conn->client_fd, 400, "Bad Request");
                                close_connection(&state, i);
                                continue;
                            }
                            
                            snprintf(conn->url, 512, "http://%s%s", host, path);
                            printf("[Worker %d] Request: %s\n", thread_id, conn->url);
                            
                            pthread_mutex_lock(&cache_mutex);
                            CacheEntry *cached = get_entry(cache, conn->url);
                            if (cached) {
                                printf("[Worker %d] Cache HIT\n", thread_id);
                                send(conn->client_fd, cached->data, cached->data_size, 0);
                                pthread_mutex_unlock(&cache_mutex);
                                
                                if (!client_wants_keepalive(conn->req_buf)) {
                                    close_connection(&state, i);
                                } else {
                                    conn->state = 0;
                                    conn->req_len = 0;
                                    conn->req_sent = 0;
                                    memset(conn->req_buf, 0, BUFFER_SIZE);
                                }
                                continue;
                            }
                            pthread_mutex_unlock(&cache_mutex);
                            
                            printf("[Worker %d] Cache MISS\n", thread_id);
                            conn->server_fd = connect_to_server(host, 80);
                            if (conn->server_fd < 0) {
                                send_error_response(conn->client_fd, 502, "Bad Gateway");
                                close_connection(&state, i);
                                continue;
                            }
                            
                            if (conn->server_fd > local_max_fd) {
                                local_max_fd = conn->server_fd;
                            }
                            
                            conn->state = 1;
                        }
                    }
                    break;
                    
                case CONNECTING:
                    if (conn->server_fd >= 0 && FD_ISSET(conn->server_fd, &state.write_fds)) {
                        int error = 0;
                        socklen_t len = sizeof(error);
                        if (getsockopt(conn->server_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
                            send_error_response(conn->client_fd, 502, "Bad Gateway");
                            close_connection(&state, i);
                            continue;
                        }
                        conn->state = 2;
                    }
                    __attribute__((fallthrough));
                    
                case SEND_REQ:
                    if (conn->server_fd >= 0 && FD_ISSET(conn->server_fd, &state.write_fds)) {
                        ssize_t sent = send(conn->server_fd, conn->req_buf + conn->req_sent, 
                                           conn->req_len - conn->req_sent, 0);
                        if (sent < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                            perror("[Worker] send to server");
                            close_connection(&state, i);
                            continue;
                        }
                        
                        conn->req_sent += sent;
                        
                        if (conn->req_sent >= conn->req_len) {
                            conn->state = 3;
                            conn->resp_len = 0;
                            conn->resp_sent = 0;
                        }
                    }
                    break;
                    
                case RECV_RESP:
                    if (conn->server_fd >= 0 && FD_ISSET(conn->server_fd, &state.read_fds)) {
                        if (conn->resp_len + BUFFER_SIZE > conn->resp_cap) {
                            conn->resp_cap = conn->resp_cap ? conn->resp_cap * 2 : BUFFER_SIZE * 2;
                            char *new_buf = realloc(conn->resp_buf, conn->resp_cap);
                            if (!new_buf) {
                                perror("[Worker] realloc");
                                close_connection(&state, i);
                                continue;
                            }
                            conn->resp_buf = new_buf;
                        }
                        
                        ssize_t n = recv(conn->server_fd, conn->resp_buf + conn->resp_len, 
                                        BUFFER_SIZE, 0);
                        if (n < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                            perror("[Worker] recv from server");
                            close_connection(&state, i);
                            continue;
                        }
                        
                        if (n == 0) {
                            conn->server_wants_close = 1;
                            goto response_complete;
                        }
                        
                        conn->resp_len += n;
                        
                        if (conn->headers_end_pos == 0) {
                            ssize_t headers_end = parse_response_headers(conn->resp_buf, 
                                                                        conn->resp_len, 
                                                                        &conn->expected_body_len);
                            if (headers_end > 0) {
                                conn->headers_end_pos = headers_end;
                                
                                const char *cl = get_header_value(conn->resp_buf, "Connection");
                                if (cl && strcasecmp(cl, "close") == 0) {
                                    conn->server_wants_close = 1;
                                }
                            }
                        }
                        
                        if (conn->headers_end_pos > 0 && conn->expected_body_len > 0) {
                            size_t body_received = conn->resp_len - conn->headers_end_pos;
                            if (body_received >= conn->expected_body_len) {
                                goto response_complete;
                            }
                        }
                        
                        continue;
                        
                    response_complete:
                        if (conn->resp_len > 0) {
                            pthread_mutex_lock(&cache_mutex);
                            char *url_copy = strdup(conn->url);
                            char *data_copy = malloc(conn->resp_len);
                            if (url_copy && data_copy) {
                                memcpy(data_copy, conn->resp_buf, conn->resp_len);
                                CacheEntry *entry = create_entry(data_copy, conn->resp_len, url_copy);
                                CacheEntry *removed = add_entry(cache, entry);
                                if (removed) free_entry(removed);
                            } else {
                                free(url_copy);
                                free(data_copy);
                            }
                            pthread_mutex_unlock(&cache_mutex);
                        }
                        
                        conn->state = 4;
                        conn->resp_sent = 0;
                    }
                    break;
                    
                case SEND_RESP:
                    if (conn->client_fd >= 0 && FD_ISSET(conn->client_fd, &state.write_fds)) {
                        ssize_t sent = send(conn->client_fd, conn->resp_buf + conn->resp_sent, 
                                           conn->resp_len - conn->resp_sent, 0);
                        if (sent < 0) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                            perror("[Worker] send to client");
                            close_connection(&state, i);
                            continue;
                        }
                        
                        conn->resp_sent += sent;
                        
                        if (conn->resp_sent >= conn->resp_len) {
                            int keep_alive = client_wants_keepalive(conn->req_buf) && !conn->server_wants_close;
                            
                            if (keep_alive) {
                                conn->state = 0;
                                conn->req_len = 0;
                                conn->req_sent = 0;
                                conn->resp_len = 0;
                                conn->resp_sent = 0;
                                conn->headers_end_pos = 0;
                                conn->expected_body_len = 0;
                                conn->server_wants_close = 0;
                                conn->url[0] = '\0';
                                if (conn->resp_buf) {
                                    free(conn->resp_buf);
                                    conn->resp_buf = NULL;
                                    conn->resp_cap = 0;
                                }
                                memset(conn->req_buf, 0, BUFFER_SIZE);
                            } else {
                                close_connection(&state, i);
                            }
                        }
                    }
                    break;
            }
        }
    }
    
    for (int i = 0; i < MAX_CONNECTIONS_PER_THREAD; i++) {
        if (state.connections[i].active) {
            close_connection(&state, i);
        }
    }
    
    free_thread_state(&state);
    
    printf("[Worker %d] Shutdown complete\n", thread_id);
    return NULL;
}

