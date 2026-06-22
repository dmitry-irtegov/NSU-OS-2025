#ifndef PROXY_UTILS_H
#define PROXY_UTILS_H

#include "common.h"

void safe_close(int fd);
void parse_request(const char *buf, size_t len, char *method, size_t method_sz,
                   char *url, size_t url_sz, char *host, size_t host_sz,
                   int *port, char *path, size_t path_sz);
int resolve_host(const char *host, int port, struct sockaddr_in *addr);
int is_status_200(const char *headers);

#endif /* PROXY_UTILS_H */
