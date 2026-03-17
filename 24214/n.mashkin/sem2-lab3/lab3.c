#include <pthread.h>
#include <stdio.h>
#include <string.h>

void *thread(void *arg) {
    char **strs = (char **)arg;
    while (*strs) {
        printf("%s\n", *strs);
        strs++;
    }
}

int main() {
    pthread_t thr1, thr2, thr3, thr4;
    char *arg1[] = {"lol", "kek", "cheburek", NULL};
    char *arg2[] = {"I", "use", "Arch", "btw", NULL};
    char *arg3[] = {"U", "W", "U", NULL};
    char *arg4[] = {"Sun", "OS", NULL};

    int res;
    if ((res = pthread_create(&thr1, NULL, thread, arg1)) != 0) {
        fprintf(stderr, "pthread_create: %s", strerror(res));
        return 1;
    }
    if ((res = pthread_create(&thr2, NULL, thread, arg2)) != 0) {
        fprintf(stderr, "pthread_create: %s", strerror(res));
        return 1;
    }
    if ((res = pthread_create(&thr3, NULL, thread, arg3)) != 0) {
        fprintf(stderr, "pthread_create: %s", strerror(res));
        return 1;
    }
    if ((res = pthread_create(&thr4, NULL, thread, arg4)) != 0) {
        fprintf(stderr, "pthread_create: %s", strerror(res));
        return 1;
    }

    if ((res = pthread_join(thr1, NULL)) != 0) {
        fprintf(stderr, "pthread_join: %s", strerror(res));
        return 1;
    }
    if ((res = pthread_join(thr2, NULL)) != 0) {
        fprintf(stderr, "pthread_join: %s", strerror(res));
        return 1;
    }
    if ((res = pthread_join(thr3, NULL)) != 0) {
        fprintf(stderr, "pthread_join: %s", strerror(res));
        return 1;
    }
    if ((res = pthread_join(thr4, NULL)) != 0) {
        fprintf(stderr, "pthread_join: %s", strerror(res));
        return 1;
    }

    return 0;
}
