#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

void check_file(const char* filename){
    FILE* file;
    if ((file = fopen(filename, "r")) == NULL) {
        perror("Не получилось открыть файл");
        exit(EXIT_FAILURE);  
    }
    printf("File opened.\n");
    fclose(file);
}

void print_uid(void) {
    uid_t realUID = getuid();
    uid_t effectiveUID = geteuid();
    printf("Real UID: %d\n", realUID);
    printf("Effective UID: %d\n", effectiveUID);
}

int main(){
    print_uid();
    
    char* filename = "1.txt";  
    check_file(filename);
    
    if (setuid(getuid()) != 0) {
        perror("Error sets the effective user ID");
        return 1;
    }

    print_uid();
    
    check_file(filename);

    exit(EXIT_SUCCESS); 
}