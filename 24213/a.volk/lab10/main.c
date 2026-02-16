#include <assert.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    if (argc == 1)
      printf("usage: %s args...\n", argv[0]);
    return 1;
  }

  pid_t pid = fork();

  if (pid == -1) {
    perror("fork");
    return 1;
  }

  if (pid != 0) {
    int status;
    pid_t cpid = wait(&status);
    if (cpid == -1) {
      perror("wait");
      return 1;
    }
    assert(cpid == pid);

    if (!WIFEXITED(status)) {
      fprintf(stderr, "process didn't exited!\n");
      return 1;
    }

    printf("process exited with: %d\n", WEXITSTATUS(status));

  } else {
    execvp(argv[1], argv + 1);
    perror("execvp");
    return 1;
  }

  return 0;
}
