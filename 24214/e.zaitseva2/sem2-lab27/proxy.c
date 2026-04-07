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

#define MAX_CONNECTIONS 510
#define BUFFER_SIZE 4096
#define LISTEN_BACKLOG 510
#define INVALID_FD (-1)

typedef struct Buffer {
  char data[BUFFER_SIZE];
  size_t start;
  size_t end;
} Buffer;

typedef struct Connection {
  int client_fd;
  int target_fd;
  int client_eof;
  int target_eof;
  Buffer c2t;
  Buffer t2c;
} Connection;

static void buffer_init(Buffer *buf) {
  buf->start = 0;
  buf->end = 0;
}

static size_t buffer_size(const Buffer *buf) {
  return buf->end - buf->start;
}

static size_t buffer_space(const Buffer *buf) {
  return BUFFER_SIZE - buf->end;
}

static void buffer_compact(Buffer *buf) {
  size_t sz;

  if (buf->start == 0) {
    return;
  }

  sz = buffer_size(buf);
  if (sz > 0) {
    memmove(buf->data, buf->data + buf->start, sz);
  }
  buf->start = 0;
  buf->end = sz;
}

static void close_fd(int *fd) {
  if (*fd != INVALID_FD) {
    close(*fd);
    *fd = INVALID_FD;
  }
}

static void remove_connection(Connection conns[], int *count, int index) {
  close_fd(&conns[index].client_fd);
  close_fd(&conns[index].target_fd);

  if (index < *count - 1) {
    memmove(&conns[index], &conns[index + 1],
            (size_t)(*count - index - 1) * sizeof(Connection));
  }
  (*count)--;
}

static int resolve_and_connect(const char *host, int port) {
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

static int read_into_buffer(int fd, Buffer *buf, int *eof_flag) {
  ssize_t n;

  if (buffer_space(buf) == 0) {
    return 0;
  }

  n = read(fd, buf->data + buf->end, buffer_space(buf));
  if (n > 0) {
    buf->end += (size_t)n;
    return 0;
  }

  if (n == 0) {
    *eof_flag = 1;
    return 0;
  }

  if (errno == EINTR) {
    return 0;
  }

  perror("read");
  return -1;
}

static int write_from_buffer(int fd, Buffer *buf) {
  ssize_t n;
  size_t sz;

  sz = buffer_size(buf);
  if (sz == 0) {
    return 0;
  }

  n = write(fd, buf->data + buf->start, sz);
  if (n > 0) {
    buf->start += (size_t)n;
    if (buf->start == buf->end) {
      buf->start = 0;
      buf->end = 0;
    }
    return 0;
  }

  if (n < 0 && errno == EINTR) {
    return 0;
  }

  perror("write");
  return -1;
}

static void init_connection(Connection *conn, int client_fd, int target_fd) {
  conn->client_fd = client_fd;
  conn->target_fd = target_fd;
  conn->client_eof = 0;
  conn->target_eof = 0;
  buffer_init(&conn->c2t);
  buffer_init(&conn->t2c);
}

int main(int argc, char *argv[]) {
  Connection conns[MAX_CONNECTIONS];
  struct pollfd pfds[1 + MAX_CONNECTIONS * 2];
  int listen_fd;
  int conn_count = 0;
  int i;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <listen_port> <target_host> <target_port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  signal(SIGPIPE, SIG_IGN);

  listen_fd = create_listener(atoi(argv[1]));
  if (listen_fd < 0) {
    return EXIT_FAILURE;
  }

  printf("Proxy listening on port %s and forwarding to %s:%s\n",
         argv[1], argv[2], argv[3]);

  while (1) {
    int nfds = 0;

    pfds[nfds].fd = listen_fd;
    pfds[nfds].events = (conn_count < MAX_CONNECTIONS) ? POLLIN : 0;
    pfds[nfds].revents = 0;
    nfds++;

    for (i = 0; i < conn_count; i++) {
      short client_events = 0;
      short target_events = 0;

      if (!conns[i].client_eof && buffer_space(&conns[i].c2t) > 0) {
        client_events |= POLLIN;
      }
      if (buffer_size(&conns[i].t2c) > 0) {
        client_events |= POLLOUT;
      }

      if (!conns[i].target_eof && buffer_space(&conns[i].t2c) > 0) {
        target_events |= POLLIN;
      }
      if (buffer_size(&conns[i].c2t) > 0) {
        target_events |= POLLOUT;
      }

      pfds[nfds].fd = conns[i].client_fd;
      pfds[nfds].events = client_events;
      pfds[nfds].revents = 0;
      nfds++;

      pfds[nfds].fd = conns[i].target_fd;
      pfds[nfds].events = target_events;
      pfds[nfds].revents = 0;
      nfds++;
    }

    if (poll(pfds, (unsigned long)nfds, -1) < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("poll");
      break;
    }

    if ((pfds[0].revents & POLLIN) != 0) {
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(client_addr);
      int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

      if (client_fd < 0) {
        perror("accept");
      } else if (conn_count >= MAX_CONNECTIONS) {
        close(client_fd);
      } else {
        int target_fd = resolve_and_connect(argv[2], atoi(argv[3]));
        if (target_fd < 0) {
          close(client_fd);
        } else {
          init_connection(&conns[conn_count], client_fd, target_fd);
          conn_count++;
        }
      }
    }

    for (i = 0; i < conn_count; ) {
      int client_idx = 1 + i * 2;
      int target_idx = client_idx + 1;
      int drop = 0;
      short cre = pfds[client_idx].revents;
      short tre = pfds[target_idx].revents;

      if ((cre & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        drop = 1;
      }
      if ((tre & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        drop = 1;
      }

      if (!drop && (cre & POLLIN) != 0) {
        if (read_into_buffer(conns[i].client_fd, &conns[i].c2t, &conns[i].client_eof) < 0) {
          drop = 1;
        }
      }

      if (!drop && (tre & POLLIN) != 0) {
        if (read_into_buffer(conns[i].target_fd, &conns[i].t2c, &conns[i].target_eof) < 0) {
          drop = 1;
        }
      }

      if (!drop && (tre & POLLOUT) != 0 && buffer_size(&conns[i].c2t) > 0) {
        if (write_from_buffer(conns[i].target_fd, &conns[i].c2t) < 0) {
          drop = 1;
        }
      }

      if (!drop && (cre & POLLOUT) != 0 && buffer_size(&conns[i].t2c) > 0) {
        if (write_from_buffer(conns[i].client_fd, &conns[i].t2c) < 0) {
          drop = 1;
        }
      }

      if (!drop && conns[i].client_eof && buffer_size(&conns[i].c2t) == 0) {
        drop = 1;
      }
      if (!drop && conns[i].target_eof && buffer_size(&conns[i].t2c) == 0) {
        drop = 1;
      }

      if (drop) {
        remove_connection(conns, &conn_count, i);
      } else {
        if (buffer_size(&conns[i].c2t) == 0) {
          buffer_compact(&conns[i].c2t);
        }
        if (buffer_size(&conns[i].t2c) == 0) {
          buffer_compact(&conns[i].t2c);
        }
        i++;
      }
    }
  }

  close(listen_fd);
  while (conn_count > 0) {
    remove_connection(conns, &conn_count, conn_count - 1);
  }

  return EXIT_SUCCESS;
}
