#define _GNU_SOURCE
#include "utils.h"
#include <stdarg.h>
#include <time.h>


void log_msg(FILE *log, const char *fmt, ...) {
    if (!log) return;
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log, "[%s] ", time_str);
    va_list args;
    va_start(args, fmt);
    vfprintf(log, fmt, args);
    va_end(args);
    fflush(log);
}

void safe_close(int fd) {
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
}

void parse_request(const char *buf, size_t len, char *method, size_t method_sz,
                   char *url, size_t url_sz, char *host, size_t host_sz,
                   int *port, char *path, size_t path_sz) {
    method[0] = '\0';
    url[0] = '\0';
    host[0] = '\0';
    *port = 80;
    path[0] = '\0';
    
    const char *p = buf;
    const char *end = buf + len;
    
    const char *space1 = memchr(p, ' ', end - p);
    if (!space1) return;
    size_t mlen = space1 - p;
    if (mlen >= method_sz) mlen = method_sz - 1;
    strncpy(method, p, mlen);
    method[mlen] = '\0';
    
    p = space1 + 1;
    const char *space2 = memchr(p, ' ', end - p);
    if (!space2) return;
    size_t ulen = space2 - p;
    if (ulen >= url_sz) ulen = url_sz - 1;
    strncpy(url, p, ulen);
    url[ulen] = '\0';
    
    const char *proto = strstr(url, "://");
    if (!proto) {
        strncpy(path, url, path_sz - 1);
        path[path_sz - 1] = '\0';
        return;
    }
    
    p = proto + 3;
    const char *slash = strchr(p, '/');
    const char *colon = strchr(p, ':');
    
    if (colon && (!slash || colon < slash)) {
        size_t hlen = colon - p;
        if (hlen >= host_sz) hlen = host_sz - 1;
        strncpy(host, p, hlen);
        host[hlen] = '\0';
        
        p = colon + 1;
        *port = atoi(p);
        if (*port == 0) *port = 80;

        while (*p && isdigit(*p)) p++;
        if (*p == '/') {
            strncpy(path, p, path_sz - 1);
            path[path_sz - 1] = '\0';
        } else {
            strncpy(path, "/", path_sz - 1);
            path[path_sz - 1] = '\0';
        }
    } else if (slash) {
        size_t hlen = slash - p;
        if (hlen >= host_sz) hlen = host_sz - 1;
        strncpy(host, p, hlen);
        host[hlen] = '\0';
        strncpy(path, slash, path_sz - 1);
        path[path_sz - 1] = '\0';
    } else {
        strncpy(host, p, host_sz - 1);
        host[host_sz - 1] = '\0';
        strncpy(path, "/", path_sz - 1);
        path[path_sz - 1] = '\0';
    }
    
    if (host[0] == '\0') {
        const char *host_hdr = strstr(buf, "\r\nHost:");
        if (!host_hdr) host_hdr = strstr(buf, "\r\nhost:");
        if (host_hdr) {
            host_hdr += 7;
            const char *hend = strstr(host_hdr, "\r\n");
            if (hend) {
                size_t hlen = hend - host_hdr;
                if (hlen >= host_sz) hlen = host_sz - 1;
                strncpy(host, host_hdr, hlen);
                host[hlen] = '\0';
                char *h = host;
                while (*h == ' ') h++;
                if (h != host) memmove(host, h, strlen(h) + 1);
            }
        }
    }
}

int resolve_host(const char *host, int port, struct sockaddr_in *addr) {
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return -1;
    }
    memcpy(addr, res->ai_addr, sizeof(*addr));
    freeaddrinfo(res);
    return 0;
}
