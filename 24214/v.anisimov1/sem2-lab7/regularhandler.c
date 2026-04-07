#include <pthread.h> 
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h> 
#include <sys/errno.h>
#include <sys/stat.h>
#include "argstruct.h"

#define BUFFERSIZE 1024

int copy(int dstFd, int srcFd) {
    char *buffer = (char *) malloc(BUFFERSIZE); 
    if (buffer == NULL) return -1; 

    for (ssize_t n = read(srcFd, buffer, BUFFERSIZE); n > 0; n = read(srcFd, buffer, BUFFERSIZE)) {
        ssize_t total_written = 0;
        while (total_written < n) {
            ssize_t sent = write(dstFd, buffer + total_written, n - total_written); 
            if (sent == -1) {
                free(buffer); 
                return -1; 
            }
            total_written += sent;
        }
    }
    free(buffer); 
    return 0; 
}

void *copyRegularFile(void *arg) {
    if (arg == NULL) return (void *) 0; 

    copypaths *cpths = (copypaths *) arg; 

    if (cpths->filepath == NULL || cpths->newpath == NULL) {
        free(cpths->filepath); 
        free(cpths->newpath); 
        free(cpths); 
        return (void *) 0; 
    }

    struct stat statistic; 
    if (stat(cpths->filepath, &statistic) != 0) {
        perror("cannot get file stat");
        free(cpths->filepath); 
        free(cpths->newpath); 
        free(cpths);
        return (void *) 0; 
    }

    int fileFd = open(cpths->filepath, O_RDONLY);
    while (fileFd == -1) {
        if (errno != EMFILE) {
            perror("cannot open file"); 
            free(cpths->filepath); 
            free(cpths->newpath); 
            free(cpths);
            return (void *) 0; 
        }
        sleep(1); 
        fileFd = open(cpths->filepath, O_RDONLY);
    }

    int copyFd = open(
        cpths->newpath, 
        O_CREAT | O_WRONLY | O_TRUNC, 
        statistic.st_mode
    ); 
    while (copyFd == -1) { 
        if (errno != EMFILE) {
            perror("cannot open file"); 
            free(cpths->filepath); 
            free(cpths->newpath); 
            free(cpths);
            close(fileFd); 
            return (void *) 0; 
        }
        sleep(1); 
        copyFd = open(
            cpths->newpath, 
            O_CREAT | O_WRONLY | O_TRUNC, 
            statistic.st_mode
        );
    }

    copy(copyFd, fileFd);

    close(fileFd); 
    close(copyFd); 

    free(cpths->filepath); 
    free(cpths->newpath); 
    free(cpths); 

    return (void *) 0; 
}
