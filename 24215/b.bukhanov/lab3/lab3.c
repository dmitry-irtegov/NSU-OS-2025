#include <stdio.h>      // printf, puts, perror, FILE, fopen, fclose
#include <stdlib.h>     // EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>     // getuid, geteuid, setuid
#include <sys/types.h>  // uid_t
#include <errno.h>      // errno

static void print_uids(void) {
    uid_t r = getuid();
    uid_t e = geteuid();
    printf("real uid: %lu, effective uid: %lu\n",
           (unsigned long)r, (unsigned long)e);
}

static int try_open_readonly(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }
    if (fclose(f) != 0) {
        perror("fclose");
        return -1;
    }
    puts("open/close OK");
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s /path/to/file\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argv[1];

    puts("== step 1 ==");
    print_uids();
    (void)try_open_readonly(path);

    puts("== step 2: setuid(getuid()) ==");
    if (setuid(getuid()) != 0) {
        perror("setuid");
        return EXIT_FAILURE;
    }

    print_uids();
    if (try_open_readonly(path) != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

