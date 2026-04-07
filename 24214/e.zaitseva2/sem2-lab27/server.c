#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CONNECTIONS 510
#define BUFFER_SIZE 4096
#define LISTEN_BACKLOG 510
#define INVALID_FD (-1)

typedef struct Client {
  int fd;
} Client;

static int create_listener(int port) {
  int fd;
  int on = 1;
  struct sockaddr_in addr;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
    perror("setsockopt(SO_REUSEADDR)");
    close(fd);
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((unsigned short)port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(fd);
    return -1;
  }

  if (listen(fd, LISTEN_BACKLOG) < 0) {
    perror("listen");
    close(fd);
    return -1;
  }

  return fd;
}

static void remove_client(Client clients[], int *count, int index) {
  close(clients[index].fd);
  if (index < *count - 1) {
    memmove(&clients[index], &clients[index + 1],
            (size_t)(*count - index - 1) * sizeof(Client));
  }
  (*count)--;
}

int main(int argc, char *argv[]) {
  Client clients[MAX_CONNECTIONS];
  struct pollfd pfds[1 + MAX_CONNECTIONS];
  int listen_fd;
  int client_count = 0;
  int i;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  signal(SIGPIPE, SIG_IGN);

  listen_fd = create_listener(atoi(argv[1]));
  if (listen_fd < 0) {
    return EXIT_FAILURE;
  }

  printf("Echo server listening on port %s\n", argv[1]);

  while (1) {
    pfds[0].fd = listen_fd;
    pfds[0].events = (client_count < MAX_CONNECTIONS) ? POLLIN : 0;
    pfds[0].revents = 0;

    for (i = 0; i < client_count; i++) {
      pfds[i + 1].fd = clients[i].fd;
      pfds[i + 1].events = POLLIN;
      pfds[i + 1].revents = 0;
    }

    if (poll(pfds, (unsigned long)(client_count + 1), -1) < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("poll");
      break;
    }

    if ((pfds[0].revents & POLLIN) != 0) {
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(client_addr);
      int fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

      if (fd < 0) {
        perror("accept");
      } else if (client_count >= MAX_CONNECTIONS) {
        close(fd);
      } else {
        clients[client_count].fd = fd;
        client_count++;
      }
    }

    for (i = 0; i < client_count; ) {
      char buffer[BUFFER_SIZE];
      short re = pfds[i + 1].revents;
      int drop = 0;

      if ((re & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        drop = 1;
      }

      if (!drop && (re & POLLIN) != 0) {
        ssize_t n = read(clients[i].fd, buffer, sizeof(buffer));
        if (n > 0) {
          ssize_t sent = 0;
          while (sent < n) {
            ssize_t m = write(clients[i].fd, buffer + sent, (size_t)(n - sent));
            if (m > 0) {
              sent += m;
            } else if (m < 0 && errno == EINTR) {
              continue;
            } else {
              perror("write");
              drop = 1;
              break;
            }
          }
        } else {
          drop = 1;
        }
      }

      if (drop) {
        remove_client(clients, &client_count, i);
      } else {
        i++;
      }
    }
  }

  close(listen_fd);
  while (client_count > 0) {
    remove_client(clients, &client_count, client_count - 1);
  }

  return EXIT_SUCCESS;
}
