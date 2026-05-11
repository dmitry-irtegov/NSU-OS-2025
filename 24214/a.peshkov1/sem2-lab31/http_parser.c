#include "http_parser.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static char* strcasestr_posix(const char *haystack, const char *needle) {
    if (!needle || *needle == '\0') return (char*)haystack;
    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++;
            n++;
        }
        if (*n == '\0') return (char*)haystack;
        haystack++;
    }
    return NULL;
}

void init_http_response(HttpResponse *res) {
    res->http_status = 0;
    res->content_length = -1;
    res->headers_received = 0;
    res->body_start_offset = 0;
    res->body_received = 0;
    res->is_chunked = 0;
}

void parse_http_response(HttpResponse *res, char *buffer, int len) {
    if (res->headers_received) {
        res->body_received = len - res->body_start_offset;
        return;
    }

    buffer[len] = '\0';
    char *headers_end = strstr(buffer, "\r\n\r\n");

    if (headers_end) {
        res->headers_received = 1;
        res->body_start_offset = (headers_end + 4) - buffer;

        sscanf(buffer, "HTTP/%*f %d", &res->http_status);

        char *cl_header = strcasestr_posix(buffer, "Content-Length:");
        if (cl_header) {
            sscanf(cl_header, "Content-Length: %ld", &res->content_length);
        }

        char *te_header = strcasestr_posix(buffer, "Transfer-Encoding:");
        if (te_header && strcasestr_posix(te_header, "chunked")) {
            res->is_chunked = 1;
        }

        res->body_received = len - res->body_start_offset;
    }
}

int is_chunked_response_complete(const char *body, int body_len) {
    if (body_len < 5) return 0;
    const char *end_marker = "\r\n0\r\n\r\n";
    if (strstr(body + body_len - 10, end_marker)) {
         return 1;
    }
    return 0;
}