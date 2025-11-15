#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>

#define SOCKETNAME "./mysocket"
#define BUFFSIZE 101

typedef struct sockaddr_un sockadd;

int initSocketAddress(sockadd *addr, const char *path) {
    addr->sun_family = AF_UNIX; 
    char *res = strcpy(addr->sun_path, path); 
    int addrLen = (int) (sizeof(*addr)); // calculate addr lenght
    return addrLen;
}

int main() {

    int clientFD = socket(AF_UNIX, SOCK_STREAM, 0); 
    if (clientFD == -1) {
        perror("CLIENT: socket creating error\n");
        return 0;
    } 

    sockadd address; // creating address structure 
    int addrLen = initSocketAddress(&address, SOCKETNAME); // get address length

    int connectionCode = connect(clientFD, (struct sockaddr*) &address, addrLen); 
    if (connectionCode == -1) {
        perror("CLIENT: connection error\n");
        close(clientFD); 
        return 0; 
    }

    char string[BUFFSIZE]; 
    size_t stringSize; 

    while (fgets(string, BUFFSIZE, stdin) != NULL) {

        stringSize = strnlen(string, BUFFSIZE); 

        if (stringSize > 0 && string[stringSize - 1] == '\n') 
            string[stringSize - 1] = '\0'; 

        if (send(clientFD, string, BUFFSIZE, 0) == -1) { // equivalent to write(2)
            perror("CLIENT: sending error\n");
            close(clientFD);
            return 0; 
        }
    }

    close(clientFD); 
    return 0; 

}
