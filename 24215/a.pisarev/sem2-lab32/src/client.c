#include "client.h"
#include "cache.h"
#include "upstream.h"
#include "utils.h"

static void finish_client(Client *client) {
    if (!client) return;
    client_disconnect(client);
    free(client);
    
    pthread_mutex_lock(&threads_mutex);
    active_threads--;
    pthread_cond_signal(&threads_cond);
    pthread_mutex_unlock(&threads_mutex);
}

void *client_thread(void *arg) {
    Client *client = (Client *)arg;
    FILE *log = client->log;
    int cache_idx = -1;
    
    log_msg(log, "[CLIENT] connected fd=%d\n", client->fd);
    
    client->req_len = 0;
    ssize_t n;
    while (client->req_len < BUF_SIZE - 1) {
        n = recv(client->fd, client->req_buf + client->req_len, BUF_SIZE - 1 - client->req_len, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            log_msg(log, "[CLIENT] recv error fd=%d errno=%d, %s\n", client->fd, errno, strerror(errno));
            finish_client(client);
            return NULL;
        }
        if (n == 0) {
            log_msg(log, "[CLIENT] client disconnected fd=%d\n", client->fd);
            finish_client(client);
            return NULL;
        }
        client->req_len += n;
        client->req_buf[client->req_len] = '\0';
        if (strstr(client->req_buf, "\r\n\r\n") != NULL) {
            break;
        }
    }
    
    if (client->req_len >= BUF_SIZE - 1) {
        log_msg(log, "[CLIENT] request too large fd=%d\n", client->fd);
        finish_client(client);
        return NULL;
    }
    
    parse_request(client->req_buf, client->req_len,
                  client->method, sizeof(client->method),
                  client->url, sizeof(client->url),
                  client->host, sizeof(client->host),
                  &client->port,
                  client->path, sizeof(client->path));
                  
    log_msg(log, "[CLIENT] request method=%s host=%s port=%d path=%s\n",
            client->method, client->host, client->port, client->path);
            
    if (strlen(client->host) == 0) {
        log_msg(log, "[CLIENT] bad request, missing host fd=%d\n", client->fd);
        client->send_len = snprintf(client->send_buf, BUF_SIZE,
                                    "HTTP/1.1 400 Bad Request\r\n"
                                    "Content-Type: text/html\r\n"
                                    "Connection: close\r\n\r\n"
                                    "<h1>400 Bad Request</h1>");
        send(client->fd, client->send_buf, client->send_len, 0);
        finish_client(client);
        return NULL;
    }
    
    cache_idx = cache_get_or_create(client->url, log);
    if (cache_idx < 0) {
        log_msg(log, "[CLIENT] cache full/unavailable, will stream without caching fd=%d\n", client->fd);
    }
    
    CacheEntry *ce = (cache_idx >= 0) ? &caches[cache_idx] : NULL;
    int is_fetcher = 0;

    if (ce) {
        pthread_mutex_lock(&cache_list_mutex);
        if (ce->state == CS_EMPTY) {
            pthread_rwlock_wrlock(&ce->rwlock);
            ce->state = CS_FETCHING;
            is_fetcher = 1;
            pthread_mutex_unlock(&cache_list_mutex);
            
            log_msg(log, "[CLIENT] fetching from upstream idx=%d\n", cache_idx);
            int res = upstream_fetch(client->host, client->port, client->path, cache_idx, client->fd, log);
            
            if (res == 1) {
                log_msg(log, "[CLIENT] upstream streamed idx=%d\n", cache_idx);
                ce->state = CS_STREAMED;
                ce->in_use = 0;
            } else if (res < 0 || ce->state == CS_EMPTY) {
                log_msg(log, "[CLIENT] upstream fetch failed idx=%d\n", cache_idx);
                ce->state = CS_ERROR;
                ce->in_use = 0;
            } else {
                log_msg(log, "[CLIENT] upstream fetch ok idx=%d\n", cache_idx);
                if (ce->body_len == 0) {
                    log_msg(log, "[CACHE] entry %d body_len=0, resetting to error\n", cache_idx);
                    ce->state = CS_ERROR;
                    ce->in_use = 0;
                } else {
                    ce->state = CS_COMPLETE;
                }
            }
            pthread_rwlock_unlock(&ce->rwlock);
            pthread_rwlock_rdlock(&ce->rwlock);
        } else {
            pthread_mutex_unlock(&cache_list_mutex);
            log_msg(log, "[CLIENT] waiting for read lock on idx=%d (state=%d)\n", cache_idx, ce->state);
            pthread_rwlock_rdlock(&ce->rwlock);
        }

        
        if (ce->state == CS_STREAMED) {
            if (is_fetcher) {
                log_msg(log, "[CLIENT] response already streamed by this thread, finishing fd=%d\n", client->fd);
                pthread_rwlock_unlock(&ce->rwlock);
                finish_client(client);
                return NULL;
            } else {
                log_msg(log, "[CLIENT] entry was streamed by another thread, streaming directly fd=%d\n", client->fd);
                pthread_rwlock_unlock(&ce->rwlock);
                upstream_fetch(client->host, client->port, client->path, -1, client->fd, log);
            }
        } else if (ce->state == CS_ERROR || ce->state == CS_EMPTY) {
            log_msg(log, "[CLIENT] cache reset/error, returning 502 fd=%d\n", client->fd);
            client->send_len = snprintf(client->send_buf, BUF_SIZE,
                                        "HTTP/1.1 502 Bad Gateway\r\n"
                                        "Content-Type: text/html\r\n"
                                        "Connection: close\r\n\r\n"
                                        "<h1>502 Bad Gateway</h1>");
            send(client->fd, client->send_buf, client->send_len, 0);
        } else {
            log_msg(log, "[CLIENT] sending cached response concurrently fd=%d h_len=%zu body_len=%zu\n",
                    client->fd, ce->h_len, ce->body_len);
            send(client->fd, ce->headers, ce->h_len, 0);
            if (ce->body_len > 0) {
                send(client->fd, ce->body, ce->body_len, 0);
            }
        }
        pthread_rwlock_unlock(&ce->rwlock);
    } else {
        //Streaming
        log_msg(log, "[CLIENT] streaming from upstream fd=%d\n", client->fd);
        upstream_fetch(client->host, client->port, client->path, -1, client->fd, log);
    }
    
    finish_client(client);
    return NULL;
}

void client_disconnect(Client *client) {
    if (client->fd >= 0) {
        log_msg(client->log, "[CLIENT] disconnecting fd=%d\n", client->fd);
        safe_close(client->fd);
        client->fd = -1;
    }
}