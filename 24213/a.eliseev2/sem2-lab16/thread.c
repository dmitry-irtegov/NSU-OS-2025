#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define ITERATIONS 10

int print_alternating(char *message, sem_t *wait_sem, sem_t *post_sem) {
    for (int i = 0; i < ITERATIONS; i++) {
        if (sem_wait(wait_sem)) {
            perror("semaphore wait failed");
            return 1;
        }
        printf("%s", message);
        if (sem_post(post_sem)) {
            perror("semaphore post failed");
            return 1;
        }
    }
    return 0;
}

static const char *name1 = "/aelsi2.lb16_1";
static const char *name2 = "/aelsi2.lb16_2";

int main() {
    sem_t *sem1 = sem_open(name1, O_CREAT | O_EXCL, 0777, 0);

    if (!sem1) {
        perror("semaphore 1 open failed");
        return 1;
    }

    sem_t *sem2 = sem_open(name2, O_CREAT | O_EXCL, 0777, 1);

    if (!sem2) {
        perror("semaphore 2 open failed");
        if (sem_unlink(name1)) {
            perror("semaphore 1 unlink failed");
        }
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        if (sem_unlink(name1)) {
            perror("semaphore 1 unlink failed");
        }
        if (sem_unlink(name2)) {
            perror("semaphore 2 unlink failed");
        }
        return 1;
    }

    char *message;
    sem_t *wait_sem, *post_sem;
    pid_t other_pid;
    if (pid) {
        message = "I'm alive (parent)\n";
        wait_sem = sem2;
        post_sem = sem1;
        other_pid = pid;
    } else {
        message = "I'm alive (child)\n";
        wait_sem = sem1;
        post_sem = sem2;
        other_pid = getppid();
    }

    int ret, error;
    ret = error = print_alternating(message, wait_sem, post_sem);

    if (error || pid) {
        if (sem_unlink(name1)) {
            perror("semaphore 1 unlink failed");
            ret = 1;
        }
        if (sem_unlink(name2)) {
            perror("semaphore 2 unlink failed");
            ret = 1;
        }
    }

    if (error) {
        if (kill(other_pid, SIGINT)) {
            perror("kill failed");
            ret = 1;
        }
    } else if (pid) {
        if (wait(NULL) == -1) {
            perror("child wait failed");
            ret = 1;
        }
    }

    return ret;
}
