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
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192
#define INITIAL_RESP_BUF_SIZE 4096

#define STATE_RECV_REQ    0
#define STATE_CONNECTING  1
#define STATE_SEND_REQ    2
#define STATE_RECV_RESP   3
#define STATE_SEND_RESP   4
#define STATE_DONE        5

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
    int body_received;
    int server_wants_close;
    
    char url[512];
    int state;
    int request_parsed;
} ClientConnection;

Cache *cache;
ClientConnection clients[MAX_CLIENTS];
fd_set read_fds, write_fds, except_fds;

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].client_fd = -1;
        clients[i].server_fd = -1;
        clients[i].req_len = 0;
        clients[i].req_sent = 0;
        clients[i].resp_buf = NULL;
        clients[i].resp_cap = 0;
        clients[i].resp_len = 0;
        clients[i].resp_sent = 0;
        clients[i].expected_body_len = 0;
        clients[i].headers_end_pos = 0;
        clients[i].body_received = 0;
        clients[i].server_wants_close = 0;
        clients[i].url[0] = '\0';
        clients[i].state = STATE_RECV_REQ;
        clients[i].request_parsed = 0;
    }
}

int add_client(int client_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_fd == -1) {
            clients[i].client_fd = client_fd;
            clients[i].server_fd = -1;
            clients[i].req_len = 0;
            clients[i].req_sent = 0;
            clients[i].resp_buf = NULL;
            clients[i].resp_cap = 0;
            clients[i].resp_len = 0;
            clients[i].resp_sent = 0;
            clients[i].expected_body_len = 0;
            clients[i].headers_end_pos = 0;
            clients[i].body_received = 0;
            clients[i].server_wants_close = 0;
            clients[i].url[0] = '\0';
            clients[i].state = STATE_RECV_REQ;
            clients[i].request_parsed = 0;
            return i;
        }
    }
    return -1;
}

void cleanup_resp_buf(ClientConnection *client) {
    if (client->resp_buf) {
        free(client->resp_buf);
        client->resp_buf = NULL;
        client->resp_cap = 0;
        client->resp_len = 0;
    }
}

void remove_client(int index) {
    if (clients[index].client_fd != -1) {
        close(clients[index].client_fd);
        clients[index].client_fd = -1;
    }
    if (clients[index].server_fd != -1) {
        close(clients[index].server_fd);
        clients[index].server_fd = -1;
    }
    cleanup_resp_buf(&clients[index]);
}

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
    
    int end_off = 4;
    const char *headers_end = strstr(buf, "\r\n\r\n");
    if (!headers_end) {
        end_off = 2;
        headers_end = strstr(buf, "\n\n");
        if (!headers_end) return -1;
    }
    
    size_t headers_len = headers_end - buf + end_off;
    if (headers_len > len) return -1;
    
    const char *cl = get_header_value(buf, "Content-Length");
    if (cl) {
        *content_length = strtoull(cl, NULL, 10);
    }
    
    return headers_len;
}

int client_wants_keepalive(ClientConnection *c) {
    const char *conn = get_header_value(c->req_buf, "Connection");
    const char *proxy_conn = get_header_value(c->req_buf, "Proxy-Connection");
    

    if (proxy_conn) {
        return strcasecmp(proxy_conn, "keep-alive") == 0;
    }
    if (conn) {
        return strcasecmp(conn, "keep-alive") == 0;
    }
    

    return (strstr(c->req_buf, "HTTP/1.1") != NULL);
}

int parse_http_request(const char *request, char *host, int host_len, char *path, int path_len) {
    char method[16];
    char url[256];
    
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

int ensure_resp_capacity(ClientConnection *client, size_t needed) {
    if (client->resp_cap >= needed) return 0;
    
    size_t new_cap = client->resp_cap ? client->resp_cap * 2 : INITIAL_RESP_BUF_SIZE;
    while (new_cap < needed) new_cap *= 2;
    
    char *new_buf = realloc(client->resp_buf, new_cap);
    if (!new_buf) return -1;
    
    client->resp_buf = new_buf;
    client->resp_cap = new_cap;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    
    cache = init_cache(10);
    init_clients();
    
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
    
    if (listen(listen_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        return 1;
    }
    

    int flags = fcntl(listen_fd, F_GETFL, 0);
    fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK);
    
    printf("Proxy server listening on port %d\n", port);
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);
        
        FD_SET(listen_fd, &read_fds);
        int max_fd = listen_fd;
        

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].client_fd == -1) continue;
            
            if (clients[i].client_fd > max_fd) max_fd = clients[i].client_fd;
            if (clients[i].server_fd > max_fd) max_fd = clients[i].server_fd;
            
            switch (clients[i].state) {
                case STATE_RECV_REQ:
                    FD_SET(clients[i].client_fd, &read_fds);
                    FD_SET(clients[i].client_fd, &except_fds);
                    break;
                case STATE_CONNECTING:
                case STATE_SEND_REQ:
                    FD_SET(clients[i].server_fd, &write_fds);
                    FD_SET(clients[i].server_fd, &except_fds);
                    break;
                case STATE_RECV_RESP:
                    FD_SET(clients[i].server_fd, &read_fds);
                    FD_SET(clients[i].server_fd, &except_fds);
                    break;
                case STATE_SEND_RESP:
                    FD_SET(clients[i].client_fd, &write_fds);
                    FD_SET(clients[i].client_fd, &except_fds);
                    break;
            }
        }
        
        int activity = select(max_fd + 1, &read_fds, &write_fds, &except_fds, NULL);
        
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }
        
        // New connection
        if (FD_ISSET(listen_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
            
            if (client_fd >= 0) {
                int flags = fcntl(client_fd, F_GETFL, 0);
                fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                
                if (add_client(client_fd) < 0) {
                    send_error_response(client_fd, 503, "Service Unavailable");
                    close(client_fd);
                }
            }
        }
        
        // State machine
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].client_fd == -1) continue;
            ClientConnection *c = &clients[i];
            

            if (FD_ISSET(c->client_fd, &except_fds) || 
                (c->server_fd >= 0 && FD_ISSET(c->server_fd, &except_fds))) {
                remove_client(i);
                continue;
            }
            
            switch (c->state) {
            case STATE_RECV_REQ:
                if (FD_ISSET(c->client_fd, &read_fds)) {
                    ssize_t n = recv(c->client_fd, c->req_buf + c->req_len, 
                                    BUFFER_SIZE - c->req_len - 1, 0);
                    if (n == 0) {
                        printf("Client %d closed connection\n", c->client_fd);
                        remove_client(i);
                        continue;
                    }

                    if (n < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue;
                        }
                        printf("Error reading from client %d: %s\n", c->client_fd, strerror(errno));
                        remove_client(i);
                        continue;
                    }
                    
                    c->req_len += n;
                    c->req_buf[c->req_len] = '\0';
                    

                    if (strstr(c->req_buf, "\r\n\r\n") || strstr(c->req_buf, "\n\n")) {
                        char host[128] = {0};
                        char path[256] = {0};
                        
                        if (parse_http_request(c->req_buf, host, sizeof(host), path, sizeof(path)) != 0) {
                            send_error_response(c->client_fd, 400, "Bad Request");
                            remove_client(i);
                            continue;
                        }
                        
                        snprintf(c->url, sizeof(c->url), "http://%s%s", host, path);

                        CacheEntry *cached = get_entry(cache, c->url);
                        if (cached) {
                            printf("Cache HIT [%d]: %s\n", c->client_fd, c->url);

                            ssize_t sent = send(c->client_fd, cached->data, cached->data_size, 0);
                            if (sent < 0 && errno != EAGAIN) {
                                remove_client(i);
                            } else {
                                c->state = STATE_DONE;
                                remove_client(i);
                            }
                            continue;
                        }
                        
                        printf("Cache MISS [%d]: %s\n", c->client_fd, c->url);
                        c->server_fd = connect_to_server(host, 80);
                        if (c->server_fd < 0) {
                            send_error_response(c->client_fd, 502, "Bad Gateway");
                            remove_client(i);
                            continue;
                        }
                        
                        c->state = STATE_CONNECTING;
                    }
                }
                break;
                
            case STATE_CONNECTING:
                if (FD_ISSET(c->server_fd, &write_fds)) {

                    int error = 0;
                    socklen_t len = sizeof(error);
                    if (getsockopt(c->server_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
                        send_error_response(c->client_fd, 502, "Bad Gateway");
                        remove_client(i);
                        continue;
                    }
                    c->state = STATE_SEND_REQ;

                }

                __attribute__((fallthrough));
                
            case STATE_SEND_REQ:
                if (FD_ISSET(c->server_fd, &write_fds)) {
                    ssize_t sent = send(c->server_fd, c->req_buf + c->req_sent, 
                                       c->req_len - c->req_sent, 0);
                    if (sent < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue;
                        }
                        remove_client(i);
                        continue;
                    }
                    
                    c->req_sent += sent;
                    
                    if (c->req_sent >= c->req_len) {

                        c->state = STATE_RECV_RESP;
                        c->resp_len = 0;
                        c->resp_sent = 0;
                        if (ensure_resp_capacity(c, INITIAL_RESP_BUF_SIZE) < 0) {
                            remove_client(i);
                            continue;
                        }
                    }
                }
                break;
                
            case STATE_RECV_RESP:
                if (FD_ISSET(c->server_fd, &read_fds)) {
                    if (ensure_resp_capacity(c, c->resp_len + BUFFER_SIZE) < 0) {
                        remove_client(i);
                        continue;
                    }

                    ssize_t n = recv(c->server_fd, c->resp_buf + c->resp_len, 
                                     BUFFER_SIZE, 0);
                    if (n < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                        remove_client(i);
                        continue;
                    }

                    if (n == 0) {
                        c->server_wants_close = 1;
                        goto response_complete;
                    }

                    c->resp_len += n;

                    if (c->headers_end_pos == 0) {
                        ssize_t headers_end = parse_response_headers(c->resp_buf, c->resp_len, &c->expected_body_len);
                        if (headers_end < 0) {
                            continue;
                        }
                        c->headers_end_pos = headers_end;

                        const char *conn = get_header_value(c->resp_buf, "Connection");
                        if (conn && strcasecmp(conn, "close") == 0) {
                            c->server_wants_close = 1;
                        }

                        if (c->expected_body_len == 0 && !c->server_wants_close) {
                            if (strncmp(c->resp_buf, "HTTP/1.0", 8) == 0) {
                                c->server_wants_close = 1;
                            }
                        }
                    }

                    if (c->headers_end_pos > 0 && c->expected_body_len > 0) {
                        c->body_received = c->resp_len - c->headers_end_pos;
                        if ((size_t)c->body_received >= c->expected_body_len) {
                            goto response_complete;
                        }
                    }

                    if (c->server_wants_close && c->headers_end_pos > 0) {
                        continue;
                    }
                    continue;

                response_complete:
                    if (c->resp_len > 12 && strncmp(c->resp_buf, "HTTP/", 5) == 0) {
                        if (strstr(c->resp_buf, " 2") != NULL) {
                            char *url_copy = strdup(c->url);
                            char *data_copy = malloc(c->resp_len);
                            if (url_copy && data_copy) {
                                memcpy(data_copy, c->resp_buf, c->resp_len);
                                CacheEntry *entry = create_entry(data_copy, c->resp_len, url_copy);
                                CacheEntry *removed = add_entry(cache, entry);
                                if (removed) {
                                    free_entry(removed);
                                }
                            } else {
                                free(url_copy);
                                free(data_copy);
                            }
                        }
                    }

                    c->state = STATE_SEND_RESP;
                    c->resp_sent = 0;
                }
                break;
                
            case STATE_SEND_RESP:
                if (FD_ISSET(c->client_fd, &write_fds)) {
                    ssize_t sent = send(c->client_fd, c->resp_buf + c->resp_sent, 
                                        c->resp_len - c->resp_sent, 0);
                    if (sent < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                        remove_client(i);
                        continue;
                    }

                    c->resp_sent += sent;

                    if (c->resp_sent >= c->resp_len) {
                        int keep_alive = client_wants_keepalive(c) && !c->server_wants_close;
                        if (keep_alive) {
                            printf("Reusing connection %d\n", c->client_fd);
                            c->state = STATE_RECV_REQ;
                            c->req_len = 0;
                            c->req_sent = 0;
                            c->resp_len = 0;
                            c->resp_sent = 0;
                            c->headers_end_pos = 0;
                            c->expected_body_len = 0;
                            c->body_received = 0;
                            c->server_wants_close = 0;
                            c->url[0] = '\0';
                            cleanup_resp_buf(c);
                        } else {
                            printf("Closing connection fd=%d (keep-alive=%d, server_close=%d)\n", 
                                   c->client_fd, client_wants_keepalive(c), c->server_wants_close);
                            c->state = STATE_DONE;
                            remove_client(i);
                        }
                    }
                }
                break;
            }
        }
    }
    
    close(listen_fd);
    return 0;
}
