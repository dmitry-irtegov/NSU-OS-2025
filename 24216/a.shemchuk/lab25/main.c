#include <ctype.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 256

int main() {
  int fd[2];
  if (pipe(fd) == -1) {
    perror("Couldn't open pipe");
    return 1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("Fork was failed");
    return 1;
  }

  if (pid > 0) {
    // parent
    if (close(fd[1])) {
      perror("Couldn't close end of pipe");
      return 1;
    }
    char buf[BUF_SIZE];
    ssize_t r;
    while ((r = read(fd[0], buf, BUF_SIZE)) > 0) {
      for (size_t i = 0; i < (size_t)r; i++) {
        buf[i] = (char)toupper(buf[i]);
      }

      if (fwrite(buf, sizeof(char), (size_t)r, stdout) != (size_t)r) {
        perror("Failed to print message");
        return 1;
      }
    }
    if (close(fd[0])) {
      perror("Couldn't close end of pipe");
      return 1;
    }
    wait(NULL);
    return 0;
  } else {
    // child
    if (close(fd[0])) {
      perror("Couldn't close end of pipe");
      return 1;
    }
    char msg[] =
        "Hi! I'm your child and I want you to uppercase this message\n";
    if (write(fd[1], msg, sizeof(msg)) == -1) {
      perror("Writing message into pipe was failed");
      return 1;
    }
    if (close(fd[1])) {
      perror("Couldn't close end of pipe");
      return 1;
    }
    return 0;
  }
}