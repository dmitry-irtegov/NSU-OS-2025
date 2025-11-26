#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct flock fl;
    fl.l_type = F_WRLCK;      
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;            

    printf("Trying to get advisory lock...\n");
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("fcntl");
        close(fd);
        return 1;
    }
    printf("Lock acquired! Starting editor...\n");

    system("vim");  

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);

    close(fd);
    return 0;
}
