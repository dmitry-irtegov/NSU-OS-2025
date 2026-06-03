#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>

typedef struct {
    char host[256];
    char port[6];
    char path[1824];
} url_t;

int url_parser(char* url, url_t* out);

typedef struct {
    char method[16];
    char version[16];
    url_t url;
} request_t;

int request_parse(char* line, request_t* out);
int request_build(request_t* request, char* buf, size_t size);

#endif
