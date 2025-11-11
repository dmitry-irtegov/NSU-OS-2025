#include <stdio.h>
#include <unistd.h>

static void print_uids(void) {
    printf("%ld %ld\n", (long)getuid(), (long)geteuid());
}

static void try_open(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) fclose(f);
    else perror("fopen");
}

int main(int argc, char **argv) {
    const char *path = argv[1];

    print_uids();
    try_open(path);

    if (setuid(getuid()) == -1) perror("setuid");

    print_uids();
    try_open(path);
    return 0;
}
