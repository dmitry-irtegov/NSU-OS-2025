#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

void print_uids(const char *tag) {
    uid_t r = getuid();
    uid_t e = geteuid();
    printf("%s: real uid = %u, effective uid = %u\n", tag, (unsigned)r, (unsigned)e);
}

int try_open(const char *filename) {
    FILE *f = fopen(filename, "r+");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }
    printf("fopen succeeded\n");
    if (fclose(f) != 0) {
        perror("fclose");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    const char *fname = "datafile";
    if (argc > 1) fname = argv[1];

    print_uids("Before open");
    if (try_open(fname) != 0) {
        printf("First open failed (maybe permission issue)\n");
    } else {
        printf("First open OK\n");
    }

    uid_t euid = geteuid();
    if (setuid(euid) != 0) {
        perror("setuid");
        printf("setuid failed — cannot change real UID to effective UID\n");
    } else {
        printf("setuid(geteuid()) succeeded\n");
    }

    print_uids("After setuid");
    if (try_open(fname) != 0) {
        printf("Second open failed\n");
    } else {
        printf("Second open OK\n");
    }

    return 0;
}
