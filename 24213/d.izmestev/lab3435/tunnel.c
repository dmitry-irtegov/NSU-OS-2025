#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>

#define MAX_CONNS 256
#define BUFFER_SIZE 511
#define FLAG 0x7E
#define ESC 0x7D
#define CMD_OPEN 0x01
#define CMD_CLOSE 0x02

typedef enum { STATE_FLAG, STATE_ID, STATE_DATA, STATE_ESC } t_state;
typedef enum { MODE_RECEIVER, MODE_SENDER } t_mode;

static t_mode mode;
static int conn_fds[MAX_CONNS];
static t_state tunnel_state = STATE_FLAG;
static uint8_t current_id = 0;
static char* target_ip;
static uint16_t target_port;
static int listen_fd = -1;

void send_to_tunnel(int tunnel_fd, uint8_t id, uint8_t *data, ssize_t len) {
    uint8_t frame[BUFFER_SIZE * 2 + 2];
    int p = 0;
    frame[p++] = FLAG;
    frame[p++] = id;
    for (int i = 0; i < len; i++) {
        if (data[i] == FLAG || data[i] == ESC) { frame[p++] = ESC; }
        frame[p++] = data[i];
    }
    frame[p++] = FLAG;
    if (write(tunnel_fd, frame, p) < 0) {
        perror("write");
        exit(1);
    }
}

void process_tunnel_data(uint8_t *raw, ssize_t n,
                         void (*handle_payload)(uint8_t, uint8_t *, size_t),
                         t_state *state, uint8_t *current_id,
                         uint8_t *decoded_buf, int *decoded_p) {
    for (int i = 0; i < n; i++) {
        uint8_t b = raw[i];
        switch (*state) {
            case STATE_FLAG:
                if (b == FLAG) {
                    *state = STATE_ID;
                }
                break;
            case STATE_ID:
                *current_id = b;
                *decoded_p = 0;
                *state = STATE_DATA;
                break;
            case STATE_DATA:
                if (b == FLAG) {
                    handle_payload(*current_id, decoded_buf, *decoded_p);
                    *state = STATE_FLAG;
                } else if (b == ESC) {
                    *state = STATE_ESC;
                } else {
                    decoded_buf[(*decoded_p)++] = b;
                }
                break;
            case STATE_ESC:
                decoded_buf[(*decoded_p)++] = b;
                *state = STATE_DATA;
                break;
        }
    }
}

int connect_to_target(const char *ip, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in addr;
    struct hostent *he = gethostbyname(ip);
    if (!he) {
        perror("gethostbyname");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }
    return fd;
}

static void handle_payload_receiver(uint8_t id, uint8_t *data, size_t len) {
    if (id == 0) {
        uint8_t cmd = data[0];
        uint8_t target_id = data[1];
        if (cmd == CMD_OPEN) {
            conn_fds[target_id] = connect_to_target(target_ip, target_port);
        } else if (cmd == CMD_CLOSE) {
            if (conn_fds[target_id] != -1) {
                if (close(conn_fds[target_id]) < 0) {
                    perror("close");
                    exit(1);
                }
                conn_fds[target_id] = -1;
            }
        }
    } else {
        if (conn_fds[id] != -1) {
            if (write(conn_fds[id], data, len) < 0) {
                perror("write to connection");
                if (close(conn_fds[id]) < 0) {
                    perror("close");
                    exit(1);
                }
                conn_fds[id] = -1;
            }
        }
    }
}

static void handle_payload_sender(uint8_t id, uint8_t *data, size_t len) {
    if (id == 0) {
        uint8_t cmd = data[0];
        uint8_t target_id = data[1];
        if (cmd == CMD_CLOSE && conn_fds[target_id] != -1) {
            if (close(conn_fds[target_id]) < 0) {
                perror("close");
                exit(1);
            }
            conn_fds[target_id] = -1;
        }
    } else {
        if (conn_fds[id] != -1) {
            if (write(conn_fds[id], data, len) < 0) {
                perror("write to client");
                if (close(conn_fds[id]) < 0) {
                    perror("close");
                    exit(1);
                }
                conn_fds[id] = -1;
            }
        }
    }
}

uint16_t parse_port(const char *port_str) {
    errno = 0;
    char *endptr;
    unsigned long port_num = strtoul(port_str, &endptr, 10);

    if (errno) {
        perror("strtoul");
        exit(1);
    } else if (port_num == 0 || port_num > 65535) {
        fprintf(stderr, "Invalid port: %s\n", port_str);
        exit(1);
    }
    return htons((uint16_t)port_num);
}

int create_server_socket(const char *port_str) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = parse_port(port_str),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    return fd;
}

int create_client_socket(const char *ip, const char *port_str) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in addr;
    struct hostent *he = gethostbyname(ip);
    if (!he) {
        perror("gethostbyname");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = parse_port(port_str);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }
    return fd;
}

static void close_connection(int conn_id, int tunnel_fd, int *conn_fds,
                             struct pollfd *fds, int fd_index) {
    uint8_t ctrl[2] = { CMD_CLOSE, (uint8_t)conn_id };
    send_to_tunnel(tunnel_fd, 0, ctrl, 2);

    if (conn_fds[conn_id] != -1) {
        if (close(fds[fd_index].fd) < 0) {
            perror("close");
            exit(1);
        }
        conn_fds[conn_id] = -1;
    }
}

void handle_connections(int tunnel_fd, struct pollfd *fds, int nfds,
                        int *id_map, int *conn_fds) {
    for (int i = 0; i < nfds; i++) {
        int conn_id = id_map[i];
        int fd = fds[i].fd;
        if (conn_fds[conn_id] == -1) {
            continue;
        }

        if (fds[i].revents & POLLHUP) {
            close_connection(conn_id, tunnel_fd, conn_fds, fds, i);
            continue;
        }

        if (fds[i].revents & POLLIN) {
            uint8_t buf[BUFFER_SIZE];
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n < 0) {
                perror("read connections");
                close_connection(conn_id, tunnel_fd, conn_fds, fds, i);
            }
            if (n == 0 && mode == MODE_SENDER) {
                close_connection(conn_id, tunnel_fd, conn_fds, fds, i);
            } else if (n > 0) {
                send_to_tunnel(tunnel_fd, (uint8_t)conn_id, buf, n);
            }
        }
    }
}

void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  Receiver mode: %s -r <listen-port> <server-ip> <server-port>\n", prog_name);
    fprintf(stderr, "  Sender mode:   %s -s <listen-port> <receiver-ip> <receiver-port>\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-r") == 0) {
        mode = MODE_RECEIVER;
    } else if (strcmp(argv[1], "-s") == 0) {
        mode = MODE_SENDER;
    } else {
        print_usage(argv[0]);
        return 1;
    }

    char* listen_port = argv[2];
    target_ip = argv[3];
    char* target_port_str = argv[4];
    target_port = parse_port(target_port_str);

    int tunnel_fd;
    memset(conn_fds, -1, sizeof(conn_fds));

    if (mode == MODE_RECEIVER) {
        int receiver_listen_fd = create_server_socket(listen_port);
        if (listen(receiver_listen_fd, 1) < 0) {
            perror("listen tunnel");
            exit(1);
        }
        tunnel_fd = accept(receiver_listen_fd, NULL, NULL);
        if (tunnel_fd < 0) {
            perror("accept tunnel");
            exit(1);
        }
        close(receiver_listen_fd);
    } else {
        listen_fd = create_server_socket(listen_port);
        if (listen(listen_fd, 255) < 0) {
            perror("listen clients");
            exit(1);
        }
        tunnel_fd = create_client_socket(target_ip, target_port_str);
    }

    struct pollfd fds[MAX_CONNS + 2];
    uint8_t decoded_buf[BUFFER_SIZE];
    int decoded_p = 0;

    while (1) {
        int nfds = 0;
        int id_map[MAX_CONNS + 2];

        if (mode == MODE_SENDER) {
            fds[nfds].fd = listen_fd;
            fds[nfds].events = POLLIN;
            id_map[nfds] = -1;
            nfds++;
        }

        fds[nfds].fd = tunnel_fd;
        fds[nfds].events = POLLIN;
        id_map[nfds] = 0;
        nfds++;

        for (int i = 1; i < MAX_CONNS; i++) {
            if (conn_fds[i] != -1) {
                fds[nfds].fd = conn_fds[i];
                fds[nfds].events = POLLIN;
                id_map[nfds] = i;
                nfds++;
            }
        }

        if (poll(fds, nfds, -1) < 0) {
            perror("poll");
            exit(1);
        }

        int tunnel_index = (mode == MODE_RECEIVER) ? 0 : 1;
        if (fds[tunnel_index].revents & POLLIN) {
            uint8_t raw[BUFFER_SIZE];
            ssize_t n = read(tunnel_fd, raw, sizeof(raw));
            if (n < 0) {
                perror("read");
                exit(1);
            }
            if (n == 0) break;

            void (*payload_handler)(uint8_t, uint8_t *, size_t) = (mode == MODE_RECEIVER) ? handle_payload_receiver : handle_payload_sender;
            process_tunnel_data(raw, n, payload_handler, &tunnel_state, &current_id, decoded_buf, &decoded_p);
        }

        if (mode == MODE_SENDER && fds[0].revents & POLLIN) {
            int new_cfd = accept(listen_fd, NULL, NULL);
            if (new_cfd < 0) {
                perror("accept client");
                exit(1);
            }
            int assigned_id = -1;
            for (int i = 1; i < MAX_CONNS; i++) {
                if (conn_fds[i] == -1) {
                    assigned_id = i;
                    break;
                }
            }
            if (assigned_id != -1) {
                conn_fds[assigned_id] = new_cfd;
                uint8_t ctrl[2] = { CMD_OPEN, (uint8_t)assigned_id };
                send_to_tunnel(tunnel_fd, 0, ctrl, 2);
            } else {
                if (close(new_cfd) < 0) {
                    perror("close extra client");
                    exit(1);
                }
            }
        }

        int start_index = (mode == MODE_RECEIVER) ? 1 : 2;
        handle_connections(tunnel_fd, &fds[start_index], nfds - start_index, &id_map[start_index], conn_fds);
    }
    return 0;
}
