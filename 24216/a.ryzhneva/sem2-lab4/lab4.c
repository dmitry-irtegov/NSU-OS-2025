#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void print_text(const char* text) {
    while(1) {
        fprintf(stderr, "%s\n", text);
        sleep(1);
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

    sleep(2);

    code = pthread_cancel(thread);
    check_code(code, argv[0], "canceling thread");

    code = pthread_join(thread, NULL);
    check_code(code, argv[0], "joining thread");

    return EXIT_SUCCESS;
}
