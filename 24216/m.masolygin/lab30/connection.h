#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_ADDRES "hidden"
#define BUF_SIZE 4

void connection(char* socket_path, int* fd, int type);