#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdlib.h>

typedef struct {
    int http_status;
    long content_length;
    int headers_received;
    size_t body_start_offset;
    long body_received;
    int is_chunked;
} HttpResponse;

void init_http_response(HttpResponse *res);

void parse_http_response(HttpResponse *res, char *buffer, int len);

int is_chunked_response_complete(const char *body, int body_len);

#endif