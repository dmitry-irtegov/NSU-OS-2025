#include <stdio.h>
#include "http_client.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <url>\n", argv[0]);
        return -1;
    }
    return http_client_fetch(argv[1]);
}
