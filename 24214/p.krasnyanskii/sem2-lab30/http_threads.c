#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFER_SIZE 4096
#define LINES_PER_PAGE 25

typedef struct {
    char *data;
    size_t size;
    size_t printed;

    int paused;
    int done;

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} shared_t;

typedef struct {
    char host[256];
    char port[6];
    char path[1024];
} url_t;

void parse_url(const char *url, url_t *res) {
    if (strncmp(url, "http://", 7) != 0) {
        fprintf(stderr, "Only http:// supported\n");
        exit(1);
    }

    const char *p = url + 7;
    const char *slash = strchr(p, '/');

    if (slash) {
        strncpy(res->host, p, slash - p);
        res->host[slash - p] = '\0';
        strcpy(res->path, slash);
    } else {
        strcpy(res->host, p);
        strcpy(res->path, "/");
    }

    strcpy(res->port, "80");

    char *colon = strchr(res->host, ':');
    if (colon) {
        strcpy(res->port, colon + 1);
        *colon = '\0';
    }
}

int connect_to_host(const char *host, const char *port) {
    struct addrinfo hints, *res, *p;
    int sockfd = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return -1;
    }

    for (p = res; p; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

void *network_thread(void *arg) {
    shared_t *shared = (shared_t *)arg;

    char buffer[BUFFER_SIZE];

    int sockfd = *(int *)shared->data; // костыль: fd передан через data

    while (1) {
        int bytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes < 0) {
            perror("recv");
            break;
        }
        if (bytes == 0) break;

        pthread_mutex_lock(&shared->mutex);

        char *tmp = realloc(shared->data, shared->size + bytes);
        if (!tmp) {
            perror("realloc");
            pthread_mutex_unlock(&shared->mutex);
            break;
        }

        shared->data = tmp;
        memcpy(shared->data + shared->size, buffer, bytes);
        shared->size += bytes;

        pthread_cond_signal(&shared->cond);
        pthread_mutex_unlock(&shared->mutex);
    }

    pthread_mutex_lock(&shared->mutex);
    shared->done = 1;
    pthread_cond_signal(&shared->cond);
    pthread_mutex_unlock(&shared->mutex);

    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <url>\n", argv[0]);
        return 1;
    }

    url_t url;
    parse_url(argv[1], &url);

    int sockfd = connect_to_host(url.host, url.port);
    if (sockfd < 0) return 1;

    char request[2048];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n",
        url.path, url.host);

    send(sockfd, request, strlen(request), 0);

    shared_t shared = {0};
    shared.data = malloc(sizeof(int)); // хак: передаём fd
    memcpy(shared.data, &sockfd, sizeof(int));

    pthread_mutex_init(&shared.mutex, NULL);
    pthread_cond_init(&shared.cond, NULL);

    pthread_t tid;
    pthread_create(&tid, NULL, network_thread, &shared);

    int lines = 0;

    while (1) {
        pthread_mutex_lock(&shared.mutex);

        while ((shared.printed >= shared.size || shared.paused) && !shared.done) {
            pthread_cond_wait(&shared.cond, &shared.mutex);
        }

        while (shared.printed < shared.size && !shared.paused) {
            char c = shared.data[shared.printed++];
            putchar(c);

            if (c == '\n') {
                lines++;
                if (lines >= LINES_PER_PAGE) {
                    shared.paused = 1;
                    printf("Press space to scroll down\n");
                    break;
                }
            }
        }

        int done = shared.done && (shared.printed >= shared.size);
        pthread_mutex_unlock(&shared.mutex);

        if (done) break;

        // обработка ввода (без select — просто read)
        if (shared.paused) {
            char c;
            if (read(STDIN_FILENO, &c, 1) > 0 && c == ' ') {
                pthread_mutex_lock(&shared.mutex);
                shared.paused = 0;
                lines = 0;
                pthread_cond_signal(&shared.cond);
                pthread_mutex_unlock(&shared.mutex);
            }
        }
    }

    pthread_join(tid, NULL);

    free(shared.data);
    pthread_mutex_destroy(&shared.mutex);
    pthread_cond_destroy(&shared.cond);

    return 0;
}

