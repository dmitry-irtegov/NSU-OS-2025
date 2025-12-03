#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/poll.h>

/*
33. Прокси-сервер
Реализуйте сервер, который принимает TCP соединения и транслирует их. Сервер
должен получать из командной строки следующие параметры:
1. Номер порта P, на котором следует слушать.
2. Имя или IP-адрес узла N, на который следует транслировать соединения.
3. Номер порта P', на который следует транслировать соединения.
Сервер принимает все входящие запросы на установление соединения на порт P. Для
каждого такого соединения он открывает соединение с портом P' на сервере N. Затем
он транслирует все данные, получаемые от клиента, серверу N, а все данные,
получаемые от сервера N – клиенту. Если сервер N или клиент разрывают соединение,
наш сервер также должен разорвать соединение. Если сервер N отказывает в
установлении соединения, следует разорвать клиентское соединение.
Сервер должен обеспечивать трансляцию 510 соединений при лимите количества
открытых файлов на процесс 1024. Сервер не должен быть многопоточным и никогда
не должен блокироваться при операциях чтения и записи. Не следует использовать
неблокирующиеся сокеты. Следует использовать select(3C) или poll(2).
*/

#define BUF_SIZE 4096
#define MAX_CONNECTIONS 1024

struct pollfd fds[MAX_CONNECTIONS];
int partners[MAX_CONNECTIONS];
int count_fds = 0;

int listen_fd;

void add_socket(int fd, int partner_fd) {
    if (count_fds >= MAX_CONNECTIONS) {
        return;
    }
    fds[count_fds].fd = fd;
    fds[count_fds].events = POLLIN;
    fds[count_fds].revents = 0;
    partners[count_fds] = partner_fd;
    count_fds++;
}

void remove_socket_at_index(int index) {
    if (index < 0 || index >= count_fds) {
        return;
    }
    close(fds[index].fd);
    fds[index] = fds[count_fds-1];
    partners[index] = partners[count_fds-1];
    count_fds--;
}

int find_index_by_fd(int fd) {
    for (int i = 0; i < count_fds; i++) {
        if (fds[i].fd == fd) {
            return i;
        }
    }
    return -1;
}

int close_pair(int fd1, int fd2) {
    int index_1 = find_index_by_fd(fd1);
    if (index_1 != -1) {
        remove_socket_at_index(index_1);
    }
    
    int index_2 = find_index_by_fd(fd2);
    if (index_2 != -1) {
        remove_socket_at_index(index_2);
    }
    
    return 1;
}

int connect_to_target(char *host, int port) {    
    struct hostent *hostent = gethostbyname(host);
    if (hostent == NULL) {
        fprintf(stderr, "Failed to resolve host: %s\n", host);
        return -1;
    }
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(port);
    memcpy(&target_addr.sin_addr, hostent->h_addr_list[0], hostent->h_length);
    
    if (connect(sock, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

int setup_server(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        return -1;
    }

    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(sock, 1024) < 0) {
        perror("listen");
        return -1;
    }

    return sock;
}

void accept_new_client(const char *target_host, int target_port) {
    int client_socket = accept(listen_fd, NULL, NULL);
    if (client_socket == -1) {
        perror("accept");
        return;
    }

    int target_socket = connect_to_target(target_host, target_port);
    if (target_socket == -1) {
        close(client_socket);
        return;
    }

    if (count_fds + 2 <= MAX_CONNECTIONS) {
        add_socket(client_socket, target_socket);
        add_socket(target_socket, client_socket);
        printf("New tunnel: Client(%d) <-> Target(%d)\n", client_socket, target_socket);
    } else {
        printf("Too many active connections. Dropping.\n");
        close(client_socket);
        close(target_socket);
    }
}

int forward_data(int source_fd, int dest_fd) {
    char buf[BUF_SIZE];
    int bytes_read = read(source_fd, buf, BUF_SIZE);

    if (bytes_read <= 0) {
        return close_pair(source_fd, dest_fd);
    }    
    if (dest_fd != -1) {
        if (send(dest_fd, buf, bytes_read, 0) == -1) {
            return close_pair(source_fd, dest_fd);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <listen_port> <target_host> <target_port>\n", argv[0]);
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);

    int listen_port = atoi(argv[1]);
    char *target_host = argv[2];
    int target_port = atoi(argv[3]);

    listen_fd = setup_server(listen_port);
    if (listen_fd < 0) {
        fprintf(stderr, "Failed to set up server on port %d\n", listen_port);
        return 1;
    }

    add_socket(listen_fd, -1);

    printf("Proxy listening on port %d, forwarding to %s:%d\n", listen_port, target_host, target_port);

    while (1) {
        if (poll(fds, count_fds, -1) < 0) {
            perror("poll");
            break;
        }
        for (int i = 0; i < count_fds; i++) {
            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                int fd = fds[i].fd;
                if (fd == listen_fd) {
                    accept_new_client(target_host, target_port);
                } else {
                    int partner = partners[i];
                    if (forward_data(fd, partner)) {
                        i--; 
                    }
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}