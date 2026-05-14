#include <stddef.h>
#include <sys/types.h>

typedef struct {
    char host[128];
    char path[256];
    int port;
    char method[16];
} http_request_t;

int http_req_parse(const char* req, http_request_t* out);
int http_header_find(const char* headers, const char* name, char* out,
                     size_t out_len);
ssize_t http_resp_parse_headers(const char* buf, size_t len,
                                size_t* content_length);

void http_send_error(int fd, int code, const char* msg);

int tcp_connect(const char* host, int port);
int tcp_set_blocking(int fd);
