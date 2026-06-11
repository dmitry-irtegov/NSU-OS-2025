#pragma once

typedef struct {
    char host[256];
    char path[1024];
    int port;
} url_t;

int url_parse(const char* url_str, url_t* url);