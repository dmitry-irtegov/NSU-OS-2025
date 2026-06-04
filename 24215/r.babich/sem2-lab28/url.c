#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "url.h"

int url_parse(const char *url_str, url_t *url) {
    if (strncmp(url_str, "http://", 7) != 0) {
        return -1;
    }

    const char *s = url_str + 7;

    const char *path = strchr(s, '/');
    const char *port = strchr(s, ':');

    if (!path) {
        strcpy(url->path, "/");
    } else {
        strncpy(url->path, path, sizeof(url->path)-1);
    }

    if (port && (!path || port < path)) {
        url->port = atoi(port + 1);
        strncpy(url->host, s, port - s);
        url->host[port - s] = '\0';
    } else {
        url->port = 80;
        if (path) {
            strncpy(url->host, s, path - s);
            url->host[path - s] = '\0';
        } else {
            strcpy(url->host, s);
        }
    }

    return 0;
}