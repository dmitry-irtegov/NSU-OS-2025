#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>


#define MSGSIZE 20

int main(){
    int fildes[2];
    pid_t pid;

    static char msg[MSGSIZE] = "soobshenie potomku";
    static char MSG[MSGSIZE];

    if (pipe(fildes) == -1){
        perror("pipe");
        return 1;
    }

    if ((pid = fork()) > 0){
        close(fildes[0]);

        if(write(fildes[1], msg, MSGSIZE) == -1){
            perror("write");
            return 1;
        }
        printf("\nPARENT: Send message %s\n", msg);

        wait(NULL);
    }
    else if(pid == 0){
        close(fildes[1]); 

        if(read(fildes[0], MSG, MSGSIZE) == -1){
            perror("read");
            return 1;
        }
        printf("\nCHILD: Accepted message %s", MSG);
        
        int len = strlen(MSG);
        for (int i = 0; i < len; i++){
            MSG[i] = toupper(MSG[i]);
        }
        
        printf("\nCHILD: Upper message %s\n", MSG);

    }
    else{

        perror("fork");
        return 1;
        
    }

    return 0;
}