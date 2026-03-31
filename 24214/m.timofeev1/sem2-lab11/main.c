#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define LINES_COUNT 10
#define MUTEX_COUNT 3

pthread_mutex_t mtx[MUTEX_COUNT];

void *child_thread(void *arg) {
  int result;

  result = pthread_mutex_lock(&mtx[2]);
  if (result != 0) {
    errno = result;
    perror("Child: initial lock mtx[2] failed");
    exit(1);
  }

  for (int i = 1; i <= LINES_COUNT; i++) {
    result = pthread_mutex_lock(&mtx[0]);
    if (result != 0) {
      errno = result;
      perror("Child: lock mtx[0] failed");
      exit(1);
    }

    printf("Child thread: line %d\n", i);

    pthread_mutex_unlock(&mtx[2]);

    pthread_mutex_lock(&mtx[1]);
    pthread_mutex_unlock(&mtx[0]);
    pthread_mutex_lock(&mtx[2]);
    pthread_mutex_unlock(&mtx[1]);
  }

  return NULL;
}

int main() {
  pthread_t thread;
  pthread_mutexattr_t mattr;
  int result;

  result = pthread_mutexattr_init(&mattr);
  if (result != 0) {
    errno = result;
    perror("pthread_mutexattr_init failed");
    exit(1);
  }

  result = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
  if (result != 0) {
    errno = result;
    perror("pthread_mutexattr_settype failed");
    exit(1);
  }

  for (int i = 0; i < MUTEX_COUNT; i++) {
    result = pthread_mutex_init(&mtx[i], &mattr);
    if (result != 0) {
      errno = result;
      perror("pthread_mutex_init failed");
      exit(1);
    }
  }
  pthread_mutexattr_destroy(&mattr);

  pthread_mutex_lock(&mtx[0]);
  pthread_mutex_lock(&mtx[1]);

  result = pthread_create(&thread, NULL, child_thread, NULL);
  if (result != 0) {
    errno = result;
    perror("pthread_create failed");
    exit(1);
  }

  for (int i = 1; i <= LINES_COUNT; i++) {
    printf("Parent thread: line %d\n", i);

    pthread_mutex_unlock(&mtx[0]);

    result = pthread_mutex_lock(&mtx[2]);
    if (result != 0) {
      errno = result;
      perror("Parent: lock mtx[2] failed");
      exit(1);
    }

    pthread_mutex_unlock(&mtx[1]);
    pthread_mutex_lock(&mtx[0]);
    pthread_mutex_unlock(&mtx[2]);
    pthread_mutex_lock(&mtx[1]);
  }

  result = pthread_join(thread, NULL);
  if (result != 0) {
    errno = result;
    perror("pthread_join failed");
    exit(1);
  }

  for (int i = 0; i < MUTEX_COUNT; i++) {
    pthread_mutex_destroy(&mtx[i]);
  }

  return 0;
}
