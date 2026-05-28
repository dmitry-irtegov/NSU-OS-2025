#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <termios.h>
//./http_client http://info.cern.ch/hypertext/WWW/TheProject.html пример
typedef enum {
    STATE_SEARCHING_HEADERS, 
    STATE_PRINTING_BODY,     
    STATE_PAUSED             
} ClientState;

// Переменная для хранения оригинальных настроек терминала
struct termios orig_termios;

// Функция для возврата терминала в нормальное состояние при выходе
void reset_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// Перевод терминала в неканонический режим (без ожидания Enter и без эха)
void set_conio_terminal_mode() {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios); 
    atexit(reset_terminal_mode);            
    
    new_termios = orig_termios;
    // Отключаем канонический режим (построчный ввод) и эхо-вывод
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <URL (например http://info.cern.ch/)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char host[256] = {0};
    char path[1024] = {0};
    
    
    char *url = argv[1];
    if (strncmp(url, "http://", 7) == 0) {
        url += 7; // Пропускаем префикс
    }

    // Ищем первый слэш, разделяющий домен и путь к файлу
    char *slash = strchr(url, '/');
    if (slash) {
        strncpy(host, url, slash - url);
        host[slash - url] = '\0'; 
        strncpy(path, slash, sizeof(path) - 1);
    } else {
        strncpy(host, url, sizeof(host) - 1);
        strcpy(path, "/");
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, "80", &hints, &res) != 0) {
        perror("Ошибка getaddrinfo");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("Ошибка socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Ошибка connect");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(res);

    // Формируем и отправляем HTTP-запрос
    char request[2048];
    snprintf(request, sizeof(request), 
             "GET %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n", path, host);
    write(sockfd, request, strlen(request));

    // Включаем специальный режим терминала (для перехвата пробела)
    set_conio_terminal_mode();

    fd_set readfds;
    int maxfd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;
    
    // Инициализация переменных для работы конечного автомата
    ClientState state = STATE_SEARCHING_HEADERS;
    int lines = 0;
    char header_window[4] = {0};

    char app_buffer[4096];
    ssize_t buf_len = 0;
    ssize_t buf_pos = 0;

    const char *pause_msg = "\033[7m Press space to scroll down... \033[0m";
    const char *clear_msg = "\r\033[K";

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Всегда слушаем клавиатуру
        if (state != STATE_PAUSED && buf_pos == buf_len) {
            FD_SET(sockfd, &readfds);   
        }

        // Блокируемся до появления данных в сети или нажатия клавиши
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Ошибка select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char c;
            if (read(STDIN_FILENO, &c, 1) > 0) {
                if (state == STATE_PAUSED && c == ' ') {
                    state = STATE_PRINTING_BODY;
                    lines = 0;
                    write(STDOUT_FILENO, clear_msg, strlen(clear_msg));
                } else if (c == 'q') { 
                    break; // Выход по нажатию 'q'
                }
            }
        }

        if (state != STATE_PAUSED && buf_pos == buf_len && FD_ISSET(sockfd, &readfds)) {
            buf_len = read(sockfd, app_buffer, sizeof(app_buffer));
            buf_pos = 0;
            
            if (buf_len <= 0) break; // Сервер закрыл соединение (конец файла) или ошибка
        }

        while (buf_pos < buf_len && state != STATE_PAUSED) {
            char c = app_buffer[buf_pos++];
            
            switch (state) {
                case STATE_SEARCHING_HEADERS:
                    // Двигаем "окно" из 4 байт, чтобы найти \r\n\r\n
                    header_window[0] = header_window[1];
                    header_window[1] = header_window[2];
                    header_window[2] = header_window[3];
                    header_window[3] = c;
                    
                    if (memcmp(header_window, "\r\n\r\n", 4) == 0) {
                        state = STATE_PRINTING_BODY; 
                    }
                    break;

                case STATE_PRINTING_BODY:
                    write(STDOUT_FILENO, &c, 1);
                    
                    if (c == '\n') {
                        lines++;
                        if (lines >= 25) {
                            write(STDOUT_FILENO, pause_msg, strlen(pause_msg));
                            state = STATE_PAUSED; 
                        }
                    }
                    break;

                case STATE_PAUSED:
                    // Логически не попадем из-за условия цикла но на всякий обрабатываем. 
                    break;
            }
        }
    }

    close(sockfd);
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}