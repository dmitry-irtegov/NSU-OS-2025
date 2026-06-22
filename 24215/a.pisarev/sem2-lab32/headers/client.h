#ifndef PROXY_CLIENT_H
#define PROXY_CLIENT_H

#include "common.h"

void *client_thread(void *arg);
void client_disconnect(Client *client);

#endif /* PROXY_CLIENT_H */

