#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SOCK_ADDR "/tmp/to_upper"
#define BUF_SIZE 256

int main() {
  int server_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("socket");
    return 1;
  }

  struct sockaddr_un server_addr;
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, SOCK_ADDR);

  if (connect(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_un))) {
    perror("connect");
    close(server_fd);
    return 1;
  }
  FILE* f = fopen("lorem_ipsum.txt", "r");
  if (!f) {
    perror("fopen");
    close(server_fd);
    return 1;
  }
  char buf[BUF_SIZE];
  size_t r;
  while ((r = fread(buf, sizeof(char), BUF_SIZE, f)) > 0) {
    if (write(server_fd, buf, r) == -1) {
      perror("write");
      close(server_fd);
      return 1;
    }
  }
  close(server_fd);
  return 0;
}