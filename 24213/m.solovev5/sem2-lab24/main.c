#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

sem_t a_detail_sem;
sem_t b_detail_sem;
sem_t c_detail_sem;
sem_t module_sem;

void handle_error(int code, char *message) {
  if (code != 0) {
    perror(message);
    exit(1);
  }
}

void handle_pthread_error(int code, char *message) {
  if (code != 0) {
    char err_buf[256];
    strerror_r(code, err_buf, sizeof(err_buf));
    fprintf(stderr, "%s: %s\n", message, err_buf);
    exit(1);
  }
}

void *a_line_routine(void *args) {
  for (;;) {
    sleep(1);
    printf("Created an A detail\n");
    handle_error(
      sem_post(&a_detail_sem),
      "A detail sem_post"
    );
  }
}

void *b_line_routine(void *args) {
  for (;;) {
    sleep(2);
    printf("Created a B detail\n");
    handle_error(
      sem_post(&b_detail_sem),
      "B detail sem_post"
    );
  }
}

void *c_line_routine(void *args) {
  for (;;) {
    sleep(3);
    printf("Created a C detail\n");
    handle_error(
      sem_post(&c_detail_sem),
      "C detail sem_post"
    );
  }
}

void *module_line_routine(void *args) {
  for (;;) {
    sem_wait(&a_detail_sem);
    sem_wait(&b_detail_sem);
    handle_error(
        sem_wait(&a_detail_sem),
        "A detail sem_wait"
    );
    handle_error(
        sem_wait(&a_detail_sem),
        "A detail sem_wait"
    );
    printf("Created a module\n");
    handle_error(
      sem_post(&module_sem),
      "Module sem_post"
    );
  }
}

void *widget_line_routine(void *args) {
  for (;;) {
    handle_error(
        sem_wait(&module_sem),
        "Module sem_wait"
    );
    handle_error(
        sem_wait(&c_detail_sem),
        "C detail sem_wait"
    );
    printf("Created a widget\n");
  }
}

int main() {
  handle_error(
    sem_init(&a_detail_sem, 0, 0),
    "A detail sem_init"
  );
  handle_error(
    sem_init(&b_detail_sem, 0, 0),
    "B detail sem_init"
  );
  handle_error(
    sem_init(&c_detail_sem, 0, 0),
    "C detail sem_init"
  );
  handle_error(
    sem_init(&module_sem, 0, 0),
    "Module sem_init"
  );

  pthread_t a_line;
  handle_pthread_error(
    pthread_create(&a_line, NULL, a_line_routine, NULL),
    "A line creating"
  );
  pthread_t b_line;
  handle_pthread_error(
    pthread_create(&b_line, NULL, b_line_routine, NULL),
    "B line creating"
  );
  pthread_t c_line;
  handle_pthread_error(
    pthread_create(&c_line, NULL, c_line_routine, NULL),
    "C line creating"
  );
  pthread_t module_line;
  handle_pthread_error(
    pthread_create(&module_line, NULL, module_line_routine, NULL),
    "Module line creating"
  );
  pthread_t widget_line;
  handle_pthread_error(
    pthread_create(&widget_line, NULL, widget_line_routine, NULL),
    "Widget line creating"
  );

  pthread_exit(0);
}
