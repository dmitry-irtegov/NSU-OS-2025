#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <stddef.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#define SOCKETNAME "./mysocket"
#define BUFFSIZE 101

typedef struct sockaddr_un sockadd;

int initSocketAddress(sockadd *addr, const char *path) {
    addr->sun_family = AF_UNIX; 
    char *res = strcpy(addr->sun_path, path); 
    int addrLen = (int) (offsetof(sockadd, sun_path) + strlen(SOCKETNAME) + 1); // calculate addr lenght
    return addrLen;
}

int main () {

    int serverFD = socket(AF_UNIX, SOCK_STREAM, 0); // creating socket
    if (serverFD == -1) {
        perror("SERVER: socket creating error\n");
        return 0;
    } 

    sockadd address; // creating address structure 
    int addrLen = initSocketAddress(&address, SOCKETNAME); // get address length

    int bindCode = bind(serverFD, (struct sockaddr*) &address, addrLen); // binding the address and the socket 
    if (bindCode == -1) {
        perror("SERVER: binding socket with the address error\n");
        close(serverFD); 
        return 0;  
    }

    int listenCode = listen(serverFD, 1); // setting the server to the listening-mode
    if (listenCode == -1) {
        perror("SERVER: setting to the listening-mode error\n");
        close(serverFD); 
        return 0;
    }

    int clientFD = accept(serverFD, NULL, NULL); 
    if (clientFD == -1) {
        perror("SERVER: accepting error\n");
        close(serverFD); 
        return 0; 
    }

    char buffer[BUFFSIZE]; 
    ssize_t recvCode = recv(clientFD, buffer, BUFFSIZE, 0); 

    while (recvCode > 0) {
        printf("String from the client: %s", buffer); 
        for (char *s = buffer; *(s) != '\0'; ++s)
            *(s) = (char) toupper((char) *(s));
        printf(" | Processed string: %s\n", buffer);
        recvCode = recv(clientFD, buffer, BUFFSIZE, 0); 
    }

    if (recvCode == -1)
        perror("SERVER: recv error\n"); 
    
    close(clientFD); 
    close(serverFD); 
    return 0; 
}
