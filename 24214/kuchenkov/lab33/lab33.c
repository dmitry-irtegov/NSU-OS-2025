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
#define MAX_FD FD_SETSIZE

fd_set main_set;
int listen_fd;
int fd_max;
int tunnel[MAX_FD];

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

    if (listen(sock, 10) < 0) {
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

    if (client_socket < MAX_FD && target_socket < MAX_FD) {
        tunnel[client_socket] = target_socket;
        tunnel[target_socket] = client_socket;

        FD_SET(client_socket, &main_set);
        FD_SET(target_socket, &main_set);
        
        if (client_socket > fd_max) {
            fd_max = client_socket;
        }
        if (target_socket > fd_max) {
            fd_max = target_socket;
        }

        printf("New tunnel: Client(%d) <-> Target(%d)\n", client_socket, target_socket);
    } else {
        fprintf(stderr, "Too many connections\n");
        close(client_socket);
        close(target_socket);
    }
}

void close_tunnel(int fd) {
    int target_fd = tunnel[fd];
    
    close(fd);
    FD_CLR(fd, &main_set);
    tunnel[fd] = -1;

    if (target_fd != -1) {
        close(target_fd);
        FD_CLR(target_fd, &main_set);
        tunnel[target_fd] = -1;
    }
}

void forward_data(int fd) {
    char buf[BUF_SIZE];
    int bytes_read = read(fd, buf, BUF_SIZE);
    int target_fd = tunnel[fd];

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            printf("Socket %d hung up\n", fd);
        } else {
            perror("read");
        }
        close_tunnel(fd);
    } else {
        if (target_fd != -1) {
            if (send(target_fd, buf, bytes_read, 0) == -1) {
                perror("send");
                close_tunnel(fd);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <listen_port> <target_host> <target_port>\n", argv[0]);
        return 1;
    }

    for (int i = 0; i < 1024; i++) {
        tunnel[i] = -1;
    }

    signal(SIGPIPE, SIG_IGN);

    int listen_port = atoi(argv[1]);
    char *target_host = argv[2];
    int target_port = atoi(argv[3]);

    listen_fd = setup_server(listen_port);
    printf("Proxy listening on port %d, forwarding to %s:%d\n", listen_port, target_host, target_port);

    FD_ZERO(&main_set);
    FD_SET(listen_fd, &main_set);
    fd_max = listen_fd;

    while (1) {
        fd_set read_set = main_set;

        if (select(fd_max + 1, &read_set, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }

        for (int i = 0; i <= fd_max; i++) {
            if (!FD_ISSET(i, &read_set)) continue;
            
            if (i == listen_fd) {
                accept_new_client(target_host, target_port);
            } else {
                forward_data(i);
            }
        }
    }

    close(listen_fd);
    return 0;
}