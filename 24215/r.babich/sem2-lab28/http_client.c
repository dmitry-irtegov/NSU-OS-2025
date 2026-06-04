#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "http_client.h"
#include "url.h"
#include "pager.h"
#include "terminal_mode.h"

int connect_to_host(const char *host, int port) {
    struct addrinfo hints = {0}, *res, *p;
    char port_str[16];

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        fprintf(stderr, "Failed to resolve host: %s\n", host);
        return -1;
    }

    int sock = -1;

    for (p = res; p; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock < 0) {
            continue;
        }

        if (connect(sock, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        close(sock);
        sock = -1;
    }

    freeaddrinfo(res);
    return sock;
}


int event_loop(int sock) {
    buffer_t buffer;
    buffer_init(&buffer);

    pager_t pager;
    pager_init(&pager, &buffer, 25);

    terminal_mode_t terminal_mode;
    terminal_mode_enable(&terminal_mode);

    fd_set rfds;
    bool sock_open = true;

    while (1) {
        if (!sock_open && pager.offset >= buffer.size) {
            break;
        }

        FD_ZERO(&rfds);
        if (sock_open) {
            FD_SET(sock, &rfds);
        }
        FD_SET(0, &rfds);

        int maxfd = sock_open ? sock : 0;

        select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (sock_open && FD_ISSET(sock, &rfds)) {
            unsigned char tmp[4096];
            ssize_t n = recv(sock, tmp, sizeof(tmp), 0);


            if (n <= 0) {
                sock_open = false;
            } else {
                buffer_append(&buffer, tmp, n);
            }
        }

        if (FD_ISSET(0, &rfds)) {
            char c;
            read(0, &c, 1);

            if (c == ' ') {
                pager.paused = false;
            }
        }
        pager_display(&pager);
    }

    terminal_mode_disable(&terminal_mode);
    close(sock);
    free(buffer.data);
    return 0;
}

int send_http_request(int sock, url_t *url) {
    char request[2048];

    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        url->path, url->host
    );

    return write(sock, request, strlen(request));
}

int http_client_fetch(const char* url_str) {
    url_t url;
    if (url_parse(url_str, &url) != 0) {
        fprintf(stderr, "Invalid URL: %s\n", url_str);
        return -1;
    }

    int socket = connect_to_host(url.host, url.port);
    if (socket < 0) {
        fprintf(stderr, "Failed to connect to host: %s\n", url.host);
        return -1;
    }

    if (send_http_request(socket, &url) < 0) {
        fprintf(stderr, "Failed to send HTTP request\n");
        close(socket);
        return -1;
    }

    return event_loop(socket);
}
