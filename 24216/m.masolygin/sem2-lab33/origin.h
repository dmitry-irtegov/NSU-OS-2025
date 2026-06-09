#ifndef ORIGIN_H
#define ORIGIN_H

#include "request.h"

int origin_connect_start(url_t* url);
void origin_set_blocking(int fd);

#endif
