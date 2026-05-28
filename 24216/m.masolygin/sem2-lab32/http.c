#define _GNU_SOURCE
#include "http.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int http_req_parse(const char* req, http_request_t* out) {
    char url[256];

    out->port = 80;

    if (sscanf(req, "%15s %255s", out->method, url) != 2) return -1;
    if (strcmp(out->method, "GET") != 0) return -1;
    if (strncmp(url, "http://", 7) != 0) return -1;

    char* host_start = url + 7;
    char* path_start = strchr(host_start, '/');

    char host_buf[sizeof(out->host)];

    if (path_start) {
        size_t hlen = (size_t)(path_start - host_start);
        if (hlen >= sizeof(out->host)) return -1;

        memcpy(host_buf, host_start, hlen);
        host_buf[hlen] = '\0';

        strncpy(out->path, path_start, sizeof(out->path));
        out->path[sizeof(out->path) - 1] = '\0';
    } else {
        strncpy(host_buf, host_start, sizeof(host_buf));
        host_buf[sizeof(host_buf) - 1] = '\0';

        strncpy(out->path, "/", sizeof(out->path));
        out->path[sizeof(out->path) - 1] = '\0';
    }

    char* colon = strchr(host_buf, ':');
    if (colon) {
        out->port = atoi(colon + 1);
        *colon = '\0';
    }

    strncpy(out->host, host_buf, sizeof(out->host));
    out->host[sizeof(out->host) - 1] = '\0';

    return 0;
}

int http_header_find(const char* headers, const char* name, char* out,
                     size_t out_len) {
    char search[128];
    snprintf(search, sizeof(search), "\r\n%s:", name);

    char* pos = strcasestr(headers, search);
    if (!pos) {
        snprintf(search, sizeof(search), "%s:", name);
        pos = strcasestr(headers, search);
        if (!pos) return 0;
    }

    pos += strlen(search);
    while (*pos == ' ' || *pos == '\t') pos++;

    char* end = pos;
    while (*end && *end != '\r' && *end != '\n') end++;

    size_t len = (size_t)(end - pos);
    if (len >= out_len) len = out_len - 1;
    strncpy(out, pos, len);
    out[len] = '\0';
    return 1;
}

ssize_t http_resp_parse_headers(const char* buf, size_t len,
                                size_t* content_length) {
    *content_length = 0;

    int end_off = 4;
    const char* headers_end = strstr(buf, "\r\n\r\n");
    if (!headers_end) {
        end_off = 2;
        headers_end = strstr(buf, "\n\n");
        if (!headers_end) return -1;
    }

    size_t hlen = (size_t)(headers_end - buf) + end_off;
    if (hlen > len) return -1;

    char cl[32];
    if (http_header_find(buf, "Content-Length", cl, sizeof(cl))) {
        *content_length = (size_t)strtoull(cl, NULL, 10);
    }

    return (ssize_t)hlen;
}

void http_send_error(int fd, int code, const char* msg) {
    char body[256];
    char response[512];

    int body_len =
        snprintf(body, sizeof(body), "<html><body><h1>%d %s</h1></body></html>",
                 code, msg);

    int len = snprintf(response, sizeof(response),
                       "HTTP/1.0 %d %s\r\n"
                       "Content-Type: text/html\r\n"
                       "Connection: close\r\n"
                       "Content-Length: %d\r\n"
                       "\r\n"
                       "%s",
                       code, msg, body_len, body);
    send(fd, response, len, 0);
}

int tcp_connect(const char* host, int port) {
    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0) return -1;

    int sock = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) break;

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;

        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);
    return sock;
}