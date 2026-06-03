#include "request.h"

#include <stdio.h>
#include <string.h>

int url_parser(char* url, url_t* out) {
    if (strncmp(url, "http://", 7) != 0) {
        return 1;
    }

    char* head = url + 7;
    char* slash = strchr(head, '/');
    char* host_end = slash ? slash : head + strlen(head);
    char* colon = memchr(head, ':', host_end - head);

    size_t host_len = (colon ? colon : host_end) - head;
    if (host_len == 0 || host_len >= sizeof(out->host)) {
        return 1;
    }
    memcpy(out->host, head, host_len);
    out->host[host_len] = '\0';

    if (colon) {
        size_t port_len = host_end - (colon + 1);
        if (port_len == 0 || port_len >= sizeof(out->port)) {
            return 1;
        }
        memcpy(out->port, colon + 1, port_len);
        out->port[port_len] = '\0';
    } else {
        strcpy(out->port, "80");
    }

    if (slash) {
        if (strlen(slash) >= sizeof(out->path)) {
            return 1;
        }
        strcpy(out->path, slash);
    } else {
        strcpy(out->path, "/");
    }

    return 0;
}

int request_parse(char* line, request_t* out) {
    char uri[2048];
    if (sscanf(line, "%15s %2047s %15s", out->method, uri, out->version) != 3) {
        return 1;
    }
    if (strcmp(out->method, "GET") != 0) {
        return 1;
    }
    if (url_parser(uri, &out->url) != 0) {
        return 1;
    }
    return 0;
}

int request_build(request_t* request, char* buf, size_t size) {
    char hostport[300];
    if (strcmp(request->url.port, "80") == 0) {
        snprintf(hostport, sizeof(hostport), "%s", request->url.host);
    } else {
        snprintf(hostport, sizeof(hostport), "%s:%s", request->url.host,
                 request->url.port);
    }

    int request_len = snprintf(buf, size,
                               "GET %s HTTP/1.0\r\n"
                               "Host: %s\r\n"
                               "Connection: close\r\n"
                               "\r\n",
                               request->url.path, hostport);
    if (request_len < 0 || request_len >= (int)size) {
        return 1;
    }

    return 0;
}
