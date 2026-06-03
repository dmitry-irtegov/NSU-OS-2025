#ifndef LISTENER_H
#define LISTENER_H

int listener_open(char* port);
int listener_accept(int listen_fd);

#endif
