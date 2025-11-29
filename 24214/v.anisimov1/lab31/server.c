#include <ctype.h> 
#include <sys/un.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <string.h>  
#include <stdlib.h> 
#include <stdio.h> 
#include <signal.h>
#include <poll.h> 
#include <unistd.h> 

#define SOCKETNAME "./mysocket"
#define BUFFSIZE 101

typedef struct pollfd pollfd; 

// МАССИВ КЛИЕНТОВ И ЕГО МЕТОДЫ 
typedef struct socketSet_s { // descriptors array for polling 
    pollfd* arr; // указатель на массив 
    nfds_t size; // количество клиентов 
    size_t capacity; // ёмкость размера
} socketSet; 

int serverIsRunning = 0; 

int initClientSet(socketSet* socketTable) { // создание массива 
    socketTable->arr = (pollfd*) malloc (sizeof(pollfd) * 10);
    if (socketTable->arr == NULL) {
        perror("SERVER: [malloc] - there isn't enough memory\n");
        return -1; 
    } 
    socketTable->capacity = 10; 
    socketTable->size = 0; 
    return 0; 
}

int addClient(socketSet* socketTable, int sockfd) {
    for (nfds_t i = 0; i < socketTable->size; ++i) {
        if (socketTable->arr[i].fd == -1) {
            socketTable->arr[i].fd = sockfd; 
            socketTable->arr[i].events = POLLIN;
            socketTable->arr[i].revents = 0; 
            return 0;
        }
    }
    if ((size_t) (socketTable->size + 1) > socketTable->capacity) {
        pollfd* newArr = realloc(socketTable->arr, sizeof(pollfd) * socketTable->capacity * 2);
        if (newArr == NULL) {
            printf("SERVER: [realloc] - there isn't enough memory\n"); 
            return -1; 
        } 
        socketTable->arr = newArr; 
        socketTable->capacity *= 2; 
    } 
    socketTable->arr[socketTable->size].fd = sockfd;
    socketTable->arr[socketTable->size].events = POLLIN; 
    socketTable->arr[socketTable->size].revents = 0; 
    socketTable->size += 1; 
    return 0; 
}

void removeClient(socketSet* socketTable, size_t clientIdx) {
    if (clientIdx >= (size_t) socketTable->size)
        return; 
    socketTable->arr[clientIdx].fd = -1;
}

// ФУНКЦИИ ДЛЯ СОЗДАНИЯ СЕРВЕРА
int initSocketAddress(struct sockaddr_un *addr, const char *path) {
    addr->sun_family = AF_UNIX;
    char *res = strcpy(addr->sun_path, path); 
    int addrLen = (int) sizeof(*addr); // calculate addr lenght
    return addrLen;
}

int turnServerOn(int serverFD, struct sockaddr_un *addr, int saddLenght, int backlog) {
    if (bind(serverFD, (struct sockaddr*) addr, saddLenght) == -1) { // связываем адрес с сокетом
        perror("SERVER: [bind] ERROR");
        return -1; 
    }
    if (listen(serverFD, backlog) == -1) { // переводим сокет в режим прослушивания 
        perror("SERVER: [listen] ERROR");
        return -1;
    }
    return 0; 
} 

// ФУНКЦИИ ДЛЯ РАБОТЫ СЕРВЕРА
void processString(char* string) {
    for (size_t i = 0; string[i] != '\0'; ++i)
        string[i] = (char) toupper((int) string[i]);
}

void serverStop(socketSet* socketTable) { // закрытие дескрипторов и очистка памяти перед выключением сервера
    for (nfds_t i = 0; i < socketTable->size; ++i) {
        if (!(socketTable->arr[i].fd < 0))
            close(socketTable->arr[i].fd);
    }
    free(socketTable->arr);  
}

void catchSIGINT(int sig) { // когда получаем SIGINT - освобождаем ресурсы 
    if (sig == SIGINT)
        serverIsRunning = 0; 
}

int main() {
    printf("SERVER: enter backlog before running: ");
    int backlog = 0; 
    if (scanf("%d", &backlog) != 1) {
        printf("Cannot get your backlog\n");
        return 0; 
    }

    int serverFD = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFD == -1) {
        perror("SERVER: [socket] ERROR"); 
        return 0; 
    }
    unlink(SOCKETNAME); 
    struct sockaddr_un serverAddr; 
    int saddLenght = initSocketAddress(&serverAddr, SOCKETNAME); 
    if (turnServerOn(serverFD, &serverAddr, saddLenght, backlog) == -1) {
        close(serverFD); 
        return 0; 
    }
 

    socketSet socketTable; 
    if (initClientSet(&socketTable) < 0) {
        printf("Cannot create a client socketTable\n"); 
        close(serverFD); 
        return 0; 
    }

    serverIsRunning = 1; 

    printf("Configuring the server...\n");
    if (sigset(SIGINT, catchSIGINT) == SIG_ERR) {
        perror("SERVER [sigset] error");
        serverStop(&socketTable); 
        return 0; 
    }

    if (addClient(&socketTable, serverFD) == -1) { //добавляем прослушиваемый сокет в таблицу
        serverStop(&socketTable);
        return 0;  
    }
    
    printf("SERVER: I`m ready\n");

    int pollCode, clientFD, readError = 0;
    int totalClients = 0, closedClients = 0; 
    ssize_t readCode;  
    char string[BUFFSIZE]; 
    while (serverIsRunning) {
        pollCode = poll(socketTable.arr, socketTable.size, -1);
        if (pollCode == -1) {
            perror("SERVER: [poll] error\n");
            break; 
        }
        if (socketTable.arr[0].revents & POLLIN) { // проверяем, что есть непринятый клиент 
            clientFD = accept(serverFD, NULL, NULL);
            if (clientFD == -1) {
                perror("SERVER: [accept] error");
                break;  
            } 
            if (addClient(&socketTable, clientFD) == -1)
                break; 
            printf("SERVER: new client added\n");
            pollCode--; 
        }
        readError = 0; 
        // обрабатываем сообщения клиентов 
        for (nfds_t i = 1; pollCode > 0 && i < socketTable.size; ++i) {
            if (socketTable.arr[i].revents & POLLIN) {
                readCode = read(socketTable.arr[i].fd, string, BUFFSIZE);
                if (readCode < 0) {
                    perror("SERVER: [read] error"); 
                    readError = 1; 
                } else if (readCode == 0) {
                    close(socketTable.arr[i].fd); 
                    removeClient(&socketTable, i); 
                } else {
                    processString(string); 
                    printf("%s\n", string);  
                }
                pollCode--; 
            }
        }
        if (readError) 
            break; 
    } 
    printf("SERVER: turning off...\n");
    serverStop(&socketTable); // закрытие прослушиваемого сокета  
    return 0; 
}
