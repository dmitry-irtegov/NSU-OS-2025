#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> 

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len) {

    struct flock lockInfo;

    lockInfo.l_type = type; 
    lockInfo.l_whence = whence; 
    lockInfo.l_start = offset; 
    lockInfo.l_len = len; 

    return(fcntl(fd, cmd, &lockInfo)); 
}

int main (int argc, char** argv) {
    
    if (argc < 2) {
        printf("There're no files from user\n");
        return 0; 
    } else if (argc > 2) {
        printf("You must pass only one file\n"); 
        return 0; 
    }

    char* filename = argv[1];  

    int fd = open(filename, O_RDWR); 
    if (fd == -1) {
        perror("ERROR: [open]");
        return 0; 
    }

    if (lock_reg(fd, F_SETLK, F_WRLCK, 0, SEEK_SET, 0) == -1) {
        perror("ERROR: [lockFile]");
        close(fd); 
        return 0;  
    }

    size_t commandSize = strlen("nano ") + strlen(filename) + 1; 
    char shellCommand[commandSize]; 
    if (snprintf(shellCommand, commandSize, "nano %s", filename) < 0) {
        perror("ERROR: [sprintf]");
        lock_reg(fd, F_SETLK, F_UNLCK, 0, SEEK_SET, 0);
        close(fd); 
        return 0; 
    }

    int editorCode = system(shellCommand); 
    if (editorCode == -1) {
        perror("Error: [system]");
        lock_reg(fd, F_SETLK, F_UNLCK, 0, SEEK_SET, 0); 
        close(fd); 
        return 0; 
    }

    printf("Editor exit code: [%d]\n", editorCode);

    if (lock_reg(fd, F_SETLK, F_UNLCK, 0, SEEK_SET, 0) == -1) {
        perror("ERROR: [lockFile]");
        return 0;  
    } 

    close(fd); 
    return 0; 

}