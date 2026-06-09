#include "conn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include "origin.h"

#define IO_CHUNK 4096

conn_t* conn_create(int fd, cache_t* cache) {
    conn_t* conn = malloc(sizeof(conn_t));
    if (conn == NULL) {
        return NULL;
    }
    conn->fd = fd;
    conn->origin_fd = -1;
    conn->state = CONN_READ_REQUEST;
    conn->cache = cache;
    conn->reqlen = 0;
    conn->out_len = 0;
    conn->out_sent = 0;
    conn->response = NULL;
    conn->response_size = 0;
    conn->response_cap = 0;
    conn->sent = 0;
    conn->header_len = 0;
    conn->content_length = 0;
    conn->origin_eof = 0;
    conn->entry = NULL;
    conn->owns_response = 0;
    return conn;
}

void conn_close_origin(conn_t* conn) {
    if (conn->origin_fd >= 0) {
        shutdown(conn->origin_fd, SHUT_RDWR);
        close(conn->origin_fd);
        conn->origin_fd = -1;
    }
}

void conn_destroy(conn_t* conn) {
    if (conn == NULL) {
        return;
    }
    conn_close_origin(conn);
    if (conn->entry != NULL) {
        cache_release(conn->cache, conn->entry);
    }
    if (conn->owns_response) {
        free(conn->response);
    }
    shutdown(conn->fd, SHUT_RDWR);
    close(conn->fd);
    free(conn);
}

void conn_fail(conn_t* conn, char* message) {
    conn_close_origin(conn);
    if (conn->owns_response) {
        free(conn->response);
    }
    conn->response = message;
    conn->response_size = strlen(message);
    conn->owns_response = 0;
    conn->sent = 0;
    conn->state = CONN_SEND_CLIENT;
}

int is_2xx(char* buf) {
    char* sp = memchr(buf, ' ', 12);
    if (sp == NULL) {
        return 0;
    }
    int code = atoi(sp + 1);
    return code >= 200 && code < 300;
}

int has_connection_close(char* data, size_t header_len) {
    for (size_t i = 0; i + 11 <= header_len; i++) {
        if (strncasecmp(data + i, "connection:", 11) == 0) {
            size_t j = i + 11;
            while (j < header_len && (data[j] == ' ' || data[j] == '\t')) {
                j++;
            }
            return j + 5 <= header_len && strncasecmp(data + j, "close", 5) == 0;
        }
    }
    return 0;
}

int response_cacheable(char* data, size_t size) {
    if (size < 12 || strncmp(data, "HTTP/", 5) != 0) {
        return 0;
    }
    if (!is_2xx(data)) {
        return 0;
    }

    size_t header_len = size;
    for (size_t i = 0; i + 4 <= size; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n' && data[i + 2] == '\r' &&
            data[i + 3] == '\n') {
            header_len = i + 4;
            break;
        }
    }

    return !has_connection_close(data, header_len);
}

size_t find_content_length(char* data, size_t header_len) {
    for (size_t i = 0; i + 15 <= header_len; i++) {
        if (strncasecmp(data + i, "content-length:", 15) == 0) {
            size_t j = i + 15;
            while (j < header_len && (data[j] == ' ' || data[j] == '\t')) {
                j++;
            }
            return (size_t)strtoull(data + j, NULL, 10);
        }
    }
    return 0;
}

void conn_parse_headers(conn_t* conn) {
    for (size_t i = 0; i < conn->response_size; i++) {
        if (i + 4 <= conn->response_size && conn->response[i] == '\r' &&
            conn->response[i + 1] == '\n' && conn->response[i + 2] == '\r' &&
            conn->response[i + 3] == '\n') {
            conn->header_len = i + 4;
            break;
        }
        if (i + 2 <= conn->response_size && conn->response[i] == '\n' &&
            conn->response[i + 1] == '\n') {
            conn->header_len = i + 2;
            break;
        }
    }
    if (conn->header_len > 0) {
        conn->content_length =
            find_content_length(conn->response, conn->header_len);
    }
}

void conn_origin_done(conn_t* conn) {
    conn->origin_eof = 1;
    conn_close_origin(conn);

    int full_body = conn->content_length > 0 &&
                    conn->response_size - conn->header_len >=
                        conn->content_length;
    if (full_body && response_cacheable(conn->response, conn->response_size)) {
        cache_add(conn->cache, conn->key, conn->response, conn->response_size);
    }

    if (conn->sent >= conn->response_size) {
        conn->state = CONN_DONE;
    }
}

int conn_active_fd(conn_t* conn) {
    switch (conn->state) {
        case CONN_READ_REQUEST:
        case CONN_SEND_CLIENT:
            return conn->fd;
        case CONN_CONNECT_ORIGIN:
        case CONN_SEND_ORIGIN:
            return conn->origin_fd;
        case CONN_STREAM:
            return conn->sent < conn->response_size ? conn->fd
                                                    : conn->origin_fd;
        default:
            return -1;
    }
}

int conn_wants_write(conn_t* conn) {
    switch (conn->state) {
        case CONN_CONNECT_ORIGIN:
        case CONN_SEND_ORIGIN:
        case CONN_SEND_CLIENT:
            return 1;
        case CONN_STREAM:
            return conn->sent < conn->response_size;
        default:
            return 0;
    }
}

void conn_advance(conn_t* conn) {
    if (conn->state == CONN_READ_REQUEST) {
        ssize_t n = recv(conn->fd, conn->reqbuf + conn->reqlen,
                         REQ_MAX - 1 - conn->reqlen, 0);
        if (n <= 0) {
            conn->state = CONN_DONE;
            return;
        }
        conn->reqlen += (size_t)n;
        conn->reqbuf[conn->reqlen] = '\0';

        if (strstr(conn->reqbuf, "\r\n\r\n") == NULL &&
            strstr(conn->reqbuf, "\n\n") == NULL) {
            if (conn->reqlen >= REQ_MAX - 1) {
                conn_fail(conn,
                          "HTTP/1.0 413 Request Entity Too Large\r\n"
                          "Connection: close\r\n\r\nRequest Entity Too Large\n");
            }
            return;
        }

        if (request_parse(conn->reqbuf, &conn->request) != 0) {
            conn_fail(conn,
                      "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n"
                      "\r\nBad Request\n");
            return;
        }

        snprintf(conn->key, sizeof(conn->key), "%s:%s%s", conn->request.url.host,
                 conn->request.url.port, conn->request.url.path);

        conn->entry = cache_get(conn->cache, conn->key);
        if (conn->entry != NULL) {
            conn->response = conn->entry->data;
            conn->response_size = conn->entry->size;
            conn->owns_response = 0;
            conn->sent = 0;
            conn->state = CONN_SEND_CLIENT;
            return;
        }

        if (request_build(&conn->request, conn->outbuf, sizeof(conn->outbuf)) !=
            0) {
            conn_fail(conn,
                      "HTTP/1.0 502 Bad Gateway\r\nConnection: close\r\n"
                      "\r\nBad Gateway\n");
            return;
        }
        conn->out_len = strlen(conn->outbuf);
        conn->out_sent = 0;

        conn->origin_fd = origin_connect_start(&conn->request.url);
        if (conn->origin_fd < 0) {
            conn_fail(conn,
                      "HTTP/1.0 502 Bad Gateway\r\nConnection: close\r\n"
                      "\r\nBad Gateway\n");
            return;
        }
        conn->state = CONN_CONNECT_ORIGIN;
        return;
    }

    if (conn->state == CONN_CONNECT_ORIGIN) {
        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(conn->origin_fd, SOL_SOCKET, SO_ERROR, &err, &len) !=
                0 ||
            err != 0) {
            conn_fail(conn,
                      "HTTP/1.0 502 Bad Gateway\r\nConnection: close\r\n"
                      "\r\nBad Gateway\n");
            return;
        }
        origin_set_blocking(conn->origin_fd);
        conn->state = CONN_SEND_ORIGIN;
        return;
    }

    if (conn->state == CONN_SEND_ORIGIN) {
        size_t remaining = conn->out_len - conn->out_sent;
        size_t chunk = remaining < IO_CHUNK ? remaining : IO_CHUNK;
        ssize_t written =
            send(conn->origin_fd, conn->outbuf + conn->out_sent, chunk, 0);
        if (written <= 0) {
            conn_fail(conn,
                      "HTTP/1.0 502 Bad Gateway\r\nConnection: close\r\n"
                      "\r\nBad Gateway\n");
            return;
        }
        conn->out_sent += (size_t)written;
        if (conn->out_sent >= conn->out_len) {
            conn->state = CONN_STREAM;
        }
        return;
    }

    if (conn->state == CONN_STREAM) {
        if (conn->sent < conn->response_size) {
            size_t remaining = conn->response_size - conn->sent;
            size_t chunk = remaining < IO_CHUNK ? remaining : IO_CHUNK;
            ssize_t written =
                send(conn->fd, conn->response + conn->sent, chunk, 0);
            if (written <= 0) {
                conn->state = CONN_DONE;
                return;
            }
            conn->sent += (size_t)written;
            if (conn->sent >= conn->response_size && conn->origin_eof) {
                conn->state = CONN_DONE;
            }
            return;
        }

        if (conn->response_size + IO_CHUNK > conn->response_cap) {
            size_t new_cap =
                conn->response_cap ? conn->response_cap * 2 : 2 * IO_CHUNK;
            char* new_response = realloc(conn->response, new_cap);
            if (new_response == NULL) {
                conn->state = CONN_DONE;
                return;
            }
            conn->response = new_response;
            conn->response_cap = new_cap;
            conn->owns_response = 1;
        }

        ssize_t n = recv(conn->origin_fd, conn->response + conn->response_size,
                         conn->response_cap - conn->response_size, 0);
        if (n > 0) {
            conn->response_size += (size_t)n;
            if (conn->header_len == 0) {
                conn_parse_headers(conn);
            }
            if (conn->content_length > 0 &&
                conn->response_size - conn->header_len >=
                    conn->content_length) {
                conn_origin_done(conn);
            }
        } else if (n == 0) {
            conn_origin_done(conn);
        } else {
            conn->state = CONN_DONE;
        }
        return;
    }

    if (conn->state == CONN_SEND_CLIENT) {
        size_t remaining = conn->response_size - conn->sent;
        size_t chunk = remaining < IO_CHUNK ? remaining : IO_CHUNK;
        ssize_t written = send(conn->fd, conn->response + conn->sent, chunk, 0);
        if (written <= 0) {
            conn->state = CONN_DONE;
            return;
        }
        conn->sent += (size_t)written;
        if (conn->sent >= conn->response_size) {
            conn->state = CONN_DONE;
        }
        return;
    }
}
