#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

void check_file(const char* filename){
    printf("Real UID: %d\nEffective UID: %d\n", getuid(), geteuid());
    FILE* file;
    if ((file = fopen(filename, "r")) == NULL) {
        perror("File dont opened");
        return;
    }
    printf("File opened.\n");
    fclose(file);
}

int main(){
    char* filename = "1.txt";  
    check_file(filename);
    
    if (setuid(getuid()) != 0) {
        perror("Error sets the effective user ID");
        exit(EXIT_FAILURE);
    }
    
    check_file(filename);

    exit(EXIT_SUCCESS); 
}