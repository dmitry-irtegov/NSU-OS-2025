#include <stdio.h>
#include <unistd.h>

static void print_ids(void) {
    uid_t ruid = getuid();
    uid_t euid = geteuid();
    printf("ruid: %d\t euid: %d\n", ruid, euid);
}

static void check_file(char *const name) {
    FILE *file = fopen(name, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }
    printf("File opened.\n");
    fclose(file);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "You need to specify a file to open.\n");
        return 1;
    }

    print_ids();
    check_file(argv[1]);

    printf("Setting euid to match ruid.\n");
    if (seteuid(getuid())) {
        perror("Failed to set effective UID.");
    }
    
    print_ids();
    check_file(argv[1]);

    return 0;
}
