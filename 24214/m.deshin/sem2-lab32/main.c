#include "proxy.h"

typedef struct ClientArg {
    int fd;
} ClientArg;

static int send_all(int fd, const char *data, size_t len) {
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, 0);

        if (n > 0) {
            sent += (size_t)n;
            continue;
        }

        if (n < 0 && errno == EINTR) {
            continue;
        }

        return -1;
    }

    return 0;
}

static void send_simple_error(int fd, int code, const char *reason, const char *body) {
    char header[1024];

    if (reason == NULL) {
        reason = "Error";
    }

    if (body == NULL) {
        body = "";
    }

    size_t body_len = strlen(body);

    int n = snprintf(header, sizeof(header),
                 "HTTP/1.0 %d %s\r\n"
                 "Content-Length: %zu\r\n"
                 "Connection: close\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n",
                 code,
                 reason,
                 body_len);

    if (n <= 0 || (size_t)n >= sizeof(header)) {
        return;
    }

    send_all(fd, header, (size_t)n);
    send_all(fd, body, body_len);
}

static int read_client_request(int fd, Buffer *request) {
    char tmp[READ_CHUNK];

    while (1) {
        if (request->len > MAX_HEADER_SIZE) {
            return -2;
        }

        if (find_header_end(request->data, request->len) >= 0) {
            return 0;
        }

        ssize_t n = recv(fd, tmp, sizeof(tmp), 0);

        if (n > 0) {
            if (buffer_append(request, tmp, (size_t)n) < 0) {
                return -1;
            }

            continue;
        }

        if (n == 0) {
            return -1;
        }

        if (errno == EINTR) {
            continue;
        }

        return -1;
    }
}

static int download_from_origin(const HttpRequest *req, Buffer *response) {
    char tmp[READ_CHUNK];
    int result = -1;
    
    Buffer origin_request;
    buffer_init(&origin_request);

    int server_fd = start_connect(req->host, req->port);
    if (server_fd < 0) {
        buffer_free(&origin_request);
        return -1;
    }

    if (build_origin_request(req, &origin_request) < 0) {
        close(server_fd);
        buffer_free(&origin_request);
        return -1;
    }

    if (send_all(server_fd, origin_request.data, origin_request.len) < 0) {
        close(server_fd);
        buffer_free(&origin_request);
        return -1;
    }

    while (1) {
        ssize_t n = recv(server_fd, tmp, sizeof(tmp), 0);

        if (n > 0) {
            if (buffer_append(response, tmp, (size_t)n) < 0) {
                result = -1;
                break;
            }

            continue;
        }

        if (n == 0) {
            result = response->len > 0 ? 0 : -1;
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        result = -1;
        break;
    }

    close(server_fd);
    buffer_free(&origin_request);

    return result;
}

static void send_cache_object(int client_fd, CacheObject *obj) {
    if (obj == NULL || obj->response.data == NULL || obj->response.len == 0) {
        send_simple_error(client_fd, 502, "Bad Gateway", "Bad Gateway\n");
        return;
    }

    send_all(client_fd, obj->response.data, obj->response.len);
}

static void cleanup(int *fd, HttpRequest *request, Buffer *client_request, Buffer *origin_response) {
    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }
    request_free(request);
    buffer_free(client_request);
    buffer_free(origin_response);
}

static void *client_thread(void *arg) {
    ClientArg *client_arg = arg;
    int client_fd = client_arg->fd;
    free(client_arg);

    Buffer client_request;
    Buffer origin_response;
    buffer_init(&client_request);
    buffer_init(&origin_response);

    HttpRequest req;
    memset(&req, 0, sizeof(req));
    req.port = 80;

    int read_result = read_client_request(client_fd, &client_request);

    if (read_result == -2) {
        send_simple_error(client_fd, 413, "Payload Too Large", "Request header is too large\n");
        cleanup(&client_fd, &req, &client_request, &origin_response);
        return NULL;
    }

    if (read_result < 0) {
        send_simple_error(client_fd, 400, "Bad Request", "Bad Request\n");
        cleanup(&client_fd, &req, &client_request, &origin_response);
        return NULL;
    }

    int http_err = 400;
    int parse_result = parse_http_request(client_request.data, client_request.len, &req, &http_err);

    if (parse_result != 1) {
        if (http_err == 405) {
            send_simple_error(client_fd, 405, "Method Not Allowed", "Only GET is supported\n");
        } else {
            send_simple_error(client_fd, 400, "Bad Request", "Bad Request\n");
        }

        cleanup(&client_fd, &req, &client_request, &origin_response);
        return NULL;
    }

    int is_owner = 0;
    CacheObject *obj = cache_get_or_reserve(req.key, &is_owner);

    if (obj == NULL) {
        send_simple_error(client_fd, 500, "Internal Server Error", "Cache error\n");
        cleanup(&client_fd, &req, &client_request, &origin_response);
        return NULL;
    }

    if (is_owner) {
        if (download_from_origin(&req, &origin_response) == 0) {
            cache_store_success(obj, &origin_response);
        } else {
            cache_store_error(obj, 502, "Bad Gateway", "Bad Gateway\n");
        }
    }

    send_cache_object(client_fd, obj);

    cache_release(obj);

    cleanup(&client_fd, &req, &client_request, &origin_response);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    const char *port = argv[1];

    int listen_fd = create_listener(port);
    if (listen_fd < 0) {
        fprintf(stderr, "cannot create listener on port %s\n", port);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "proxy is listening on port %s\n", port);

    while (1) {
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);

        int client_fd = accept(listen_fd, (struct sockaddr *)&addr, &addr_len);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("accept");
            continue;
        }

        ClientArg *arg = malloc(sizeof(*arg));
        if (arg == NULL) {
            send_simple_error(client_fd, 503, "Service Unavailable", "No memory\n");
            close(client_fd);
            continue;
        }

        arg->fd = client_fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, arg) != 0) {
            client_thread(arg);
            continue;
        }

        pthread_detach(tid);
    }

    close(listen_fd);
    cache_free_all();

    return EXIT_SUCCESS;
}
