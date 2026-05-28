#ifndef PROXY_EVENT_LOOP_H
#define PROXY_EVENT_LOOP_H
#include "common.h"

void event_loop_init(int listen_fd);
void event_loop_run(void);
void event_loop_cleanup(void);

#endif /* PROXY_EVENT_LOOP_H */