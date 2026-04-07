#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

static int connect_to_host(const char *host, int port) {
  struct hostent *he;
  struct sockaddr_in addr;
  int fd;

  he = gethostbyname(host);
  if (he == NULL || he->h_addrtype != AF_INET || he->h_length != 4) {
    fprintf(stderr, "Cannot resolve host: %s\n", host);
    return -1;
  }

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons((unsigned short)port);
  memcpy(&addr.sin_addr, he->h_addr, (size_t)he->h_length);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect");
    close(fd);
    return -1;
  }

  return fd;
}

int main(int argc, char *argv[]) {
  int fd;
  int running = 1;
  char buffer[BUFFER_SIZE];
  struct pollfd pfds[2];

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  signal(SIGPIPE, SIG_IGN);

  fd = connect_to_host(argv[1], atoi(argv[2]));
  if (fd < 0) {
    return EXIT_FAILURE;
  }

  printf("Connected to %s:%s\n", argv[1], argv[2]);

  pfds[0].fd = STDIN_FILENO;
  pfds[0].events = POLLIN;
  pfds[0].revents = 0;

  pfds[1].fd = fd;
  pfds[1].events = POLLIN;
  pfds[1].revents = 0;

  while (running) {
    if (poll(pfds, 2, -1) < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("poll");
      break;
    }

    if ((pfds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      printf("Connection closed by server/proxy\n");
      break;
    }

    if ((pfds[1].revents & POLLIN) != 0) {
      ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
      if (n > 0) {
        buffer[n] = '\0';
        printf("Response: %s", buffer);
        if (buffer[n - 1] != '\n') {
          putchar('\n');
        }
      } else if (n == 0) {
        printf("Connection closed by server/proxy\n");
        break;
      } else if (errno != EINTR) {
        perror("read");
        break;
      }
    }

    if ((pfds[0].revents & POLLIN) != 0) {
      size_t len;
      size_t sent;

      if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        running = 0;
        continue;
      }

      len = strlen(buffer);
      sent = 0;

      while (sent < len) {
        ssize_t n = write(fd, buffer + sent, len - sent);
        if (n > 0) {
          sent += (size_t)n;
        } else if (n < 0 && errno == EINTR) {
          continue;
        } else {
          perror("write");
          running = 0;
          break;
        }
      }
    }
  }

  close(fd);
  return EXIT_SUCCESS;
}