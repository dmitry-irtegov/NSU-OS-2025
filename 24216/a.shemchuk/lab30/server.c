#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

#define SOCK_ADDR "/tmp/to_upper"
#define BUF_SIZE 256

int main() {
  unlink(SOCK_ADDR);
  int server_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un server_addr;
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, SOCK_ADDR);

  if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
    perror("bind");
    close(server_fd);
    return 1;
  }
  if (listen(server_fd, 52)) {
    perror("listen");
    close(server_fd);
    return 1;
  }
  struct sockaddr_un client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
  if (client_fd == -1) {
    perror("accept");
    close(server_fd);
    return 1;
  }
  ssize_t r;

  char buf[BUF_SIZE];
  while ((r = read(client_fd, buf, BUF_SIZE - 1)) > 0) {
    for (ssize_t i = 0; i < r; i++) {
      buf[i] = toupper(buf[i]);
    }
    buf[r] = '\0';
    printf("%s", buf);
  }
  close(client_fd);
  close(server_fd);
  return 0;
}