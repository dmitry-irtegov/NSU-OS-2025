#ifndef PROXY_CLIENT_H
#define PROXY_CLIENT_H
#include "common.h"

int client_accept(int listen_fd);
void client_handle_event(int ci, short revents);
void client_disconnect(int ci);
void client_append_stream(Client *cl, const char *data, size_t len);

#endif /* PROXY_CLIENT_H */