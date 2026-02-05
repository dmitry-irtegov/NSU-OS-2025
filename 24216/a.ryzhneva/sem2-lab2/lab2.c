#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

void print_text(const char* text) {
    for(int i = 0; i < 10; i++) {
        printf("%s\n", text);
    }
}

void* thread_body(void* param) {
    print_text("Child");
    return NULL;
}

void check_code(int code, const char* name_prog, const char* action) {
    if (code != 0) {
        char buf[256];
        strerror_r(code, buf, sizeof buf);
        fprintf(stderr, "%s: %s: %s\n", name_prog, action, buf);
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    pthread_t thread;
    int code;

    code = pthread_create(&thread, NULL, thread_body, NULL);
    check_code(code, argv[0], "creating thread");

    code = pthread_join(thread, NULL);
    check_code(code, argv[0], "joining thread");

    print_text("Parent");

    return EXIT_SUCCESS;
}