#include <stdio.h> 
#include <sys/stat.h>
#include <stdlib.h> 
#include <pthread.h> 
#include <string.h> 
#include "argstruct.h"
#include "dirhandler.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("incorrect usage");
        return 0; 
    }

    char *src = argv[1]; 
    
    struct stat filestat; 
    if (stat(src, &filestat) != 0) {
        perror("cannot get file statistic"); 
        return 0; 
    }

    if (S_ISDIR(filestat.st_mode) == 0) {
        printf("file [%s] is not a directory", src);
        return 0; 
    }

    copypaths *arg = (copypaths *) malloc (sizeof(copypaths)); 
    if (arg == NULL) {
        perror("cannot allocate an argument");
        return 0; 
    }

    arg->filepath = strdup(argv[1]);
    if (arg->filepath == NULL) {
        perror("cannot save a copy"); 
        free(arg); 
        return 0; 
    }

    arg->newpath = strdup(argv[2]);
    if (arg->newpath == NULL) {
        perror("cannot save a copy"); 
        free(arg->filepath); 
        free(arg); 
        return 0; 
    }

    pthread_t thread; 
    int code = pthread_create(&thread, NULL, copyDirectory, (void *) arg); 
    if (code != 0) {
        char buff[256]; 
        strerror_r(code, buff, sizeof(buff)); 
        fprintf(stderr, "pthread_create: [%s]", buff);
        free(arg); 
        return 0; 
    }

    code = pthread_join(thread, NULL); 
    if (code != 0) {
        char buff[256]; 
        strerror_r(code, buff, sizeof(buff)); 
        fprintf(stderr, "pthread_join: [%s]", buff); 
    }

    return 0; 
}