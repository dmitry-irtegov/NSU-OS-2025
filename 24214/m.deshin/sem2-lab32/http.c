#include "proxy.h"

static char *dup_part(const char *begin, size_t len) {
    char *s = malloc(len + 1);
    if (s == NULL) {
        return NULL;
    }

    memcpy(s, begin, len);
    s[len] = '\0';

    return s;
}

ssize_t find_header_end(const char *buf, size_t len) {
    if (buf == NULL || len < 4) {
        return -1;
    }

    for (size_t i = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' &&
            buf[i + 1] == '\n' &&
            buf[i + 2] == '\r' &&
            buf[i + 3] == '\n') {
            return (ssize_t)(i + 4);
        }
    }

    return -1;
}

char *make_cache_key(const char *host, int port, const char *path) {
    if (host == NULL || path == NULL) {
        return NULL;
    }

    size_t host_len = strlen(host);
    size_t path_len = strlen(path);

    size_t total = host_len + path_len + 32;

    char *key = malloc(total);
    if (key == NULL) {
        return NULL;
    }

    snprintf(key, total, "%s:%d%s", host, port, path);

    return key;
}

static int parse_host_port(char *host, int *port) {
    if (host == NULL || port == NULL) {
        return -1;
    }

    *port = 80;

    char *colon = strrchr(host, ':');
    if (colon != NULL) {
        *colon = '\0';
        colon++;

        if (*colon == '\0') {
            return -1;
        }

        errno = 0;

        char *endptr;
        long value = strtol(colon, &endptr, 10);

        if (errno != 0 || *endptr != '\0' || value <= 0 || value > 65535) {
            return -1;
        }

        *port = (int)value;
    }

    if (host[0] == '\0') {
        return -1;
    }

    return 0;
}

static char *extract_host_header(const char *buf, size_t header_len) {
    size_t i = 0;

    while (i + 6 < header_len) {
        size_t line_start = i;

        while (i + 1 < header_len && !(buf[i] == '\r' && buf[i + 1] == '\n')) {
            i++;
        }

        if (i + 1 >= header_len) {
            break;
        }

        size_t line_len = i - line_start;

        if (line_len >= 5 && strncasecmp(buf + line_start, "Host:", 5) == 0) {
            const char *value = buf + line_start + 5;
            const char *end = buf + line_start + line_len;

            while (value < end && isspace((unsigned char)*value)) {
                value++;
            }

            while (end > value && isspace((unsigned char)*(end - 1))) {
                end--;
            }

            return dup_part(value, (size_t)(end - value));
        }

        i += 2;
    }

    return NULL;
}

const char *find_crlf(const char *buf, size_t len) {
    if (buf == NULL || len < 2) {
        return NULL;
    }

    for (size_t i = 0; i + 1 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') {
            return buf + i;
        }
    }

    return NULL;
}

int parse_http_request(const char *buf, size_t len, HttpRequest *r, int *http_err) {
    if (http_err != NULL) {
        *http_err = 400;
    }

    if (buf == NULL || r == NULL) {
        return -1;
    }

    ssize_t header_end = find_header_end(buf, len);
    if (header_end < 0) {
        return 0;
    }

    const char *line_end = find_crlf(buf, (size_t)header_end);
    if (line_end == NULL || line_end > buf + header_end) {
        return -1;
    }

    const char *p = buf;

    const char *sp1 = memchr(p, ' ', (size_t)(line_end - p));
    if (sp1 == NULL) {
        return -1;
    }

    const char *sp2 = memchr(sp1 + 1, ' ', (size_t)(line_end - sp1 - 1));
    if (sp2 == NULL) {
        return -1;
    }

    r->method = dup_part(p, (size_t)(sp1 - p));
    r->path = dup_part(sp1 + 1, (size_t)(sp2 - sp1 - 1));
    r->version = dup_part(sp2 + 1, (size_t)(line_end - sp2 - 1));

    if (r->method == NULL || r->path == NULL || r->version == NULL) {
        return -1;
    }

    if (strcmp(r->method, "GET") != 0) {
        if (http_err != NULL) {
            *http_err = 405;
        }
        return -1;
    }

    r->host = extract_host_header(buf, (size_t)header_end);
    if (r->host == NULL) {
        if (http_err != NULL) {
            *http_err = 400;
        }
        return -1;
    }

    char *host_copy = r->host;
    if (parse_host_port(host_copy, &r->port) < 0) {
        if (http_err != NULL) {
            *http_err = 400;
        }
        return -1;
    }

    r->key = make_cache_key(r->host, r->port, r->path);
    if (r->key == NULL) {
        return -1;
    }

    if (http_err != NULL) {
        *http_err = 200;
    }

    return 1;
}

static const char *origin_path(const char *path) {
    if (path == NULL) {
        return "/";
    }

    if (strncmp(path, "http://", 7) != 0) {
        return path;
    }

    const char *p = path + 7;
    const char *slash = strchr(p, '/');

    if (slash == NULL) {
        return "/";
    }

    return slash;
}

int build_origin_request(const HttpRequest *r, Buffer *out) {
    char header[4096];

    if (r == NULL || out == NULL || r->host == NULL || r->path == NULL) {
        return -1;
    }

    int n = snprintf(header, sizeof(header),
                    "GET %s HTTP/1.0\r\n"
                    "Host: %s\r\n"
                    "Connection: close\r\n"
                    "\r\n",
                    origin_path(r->path),    
                    r->host);
    
    if (n < 0 || (size_t)n >= sizeof(header)) {
        return -1;
    }

    return buffer_append(out, header, (size_t)n);
}

void request_free(HttpRequest *r) {
    if (r == NULL) {
        return;
    }

    free(r->method);
    free(r->host);
    free(r->path);
    free(r->version);
    free(r->key);

    r->method = NULL;
    r->host = NULL;
    r->path = NULL;
    r->version = NULL;
    r->key = NULL;
    r->port = 80;
}
