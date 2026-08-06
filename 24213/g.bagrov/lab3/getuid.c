#include <stdio.h>
#include <unistd.h>

static void print_uids(void) {
    printf("Ruid: %d Euid: %d\n", getuid(), geteuid());
}

static void try_open(const char *path) {
    FILE *f = fopen(path, "r");
    if (f){
        printf("file opening successful\n");
        fclose(f);
    } 
    else perror("fopen error");
}

int main(int argc, char **argv) {
    if(argc != 2){
        fprintf(stderr, "incorrect amount of arguments");
        return 1;
    }
    const char *path = argv[1];

    print_uids();
    try_open(path);

    if (setuid(getuid()) == -1){
        perror("setuid error");
        return 1;
    } 

    print_uids();
    try_open(path);
    return 0;
}
