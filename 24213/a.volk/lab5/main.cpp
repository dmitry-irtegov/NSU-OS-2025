#include <array>
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

struct table_entry_t {
  off_t offset;
  size_t len;
};

using table_t = std::vector<table_entry_t>;

void usage(char *invoke_path) { printf("%s <text file>\n", invoke_path); }

int main(int argc, char *argv[]) {

  if (argc != 2) {
    usage(argv[0]);
    return -1;
  }

  int fd = open("text.txt", O_RDONLY);
  if (fd == -1) {
    perror("failed open file");
    return -1;
  }

  const auto file_close = [](const int *const fd_p) { close(*fd_p); };
  const std::unique_ptr<int, decltype(file_close)> _fd_defer_close{&fd,
                                                                   file_close};

  table_t table{};

  constexpr size_t buff_sz = 2048;
  std::array<char, buff_sz> buff{};

  off_t prev_off = -1;
  off_t curr_off = -1;
  ssize_t read_bytes = -1;

  for (; (read_bytes = read(fd, buff.data(), buff_sz * sizeof(char))) > 0;) {
    for (off_t i = 0; i < (off_t)read_bytes; i++) {
      curr_off++;
      if (buff[i] == '\n') {
        table.emplace_back(
            table_entry_t{prev_off + 1, (size_t)(curr_off - prev_off)});
        prev_off = curr_off;
      }
    }
  }

  if (read_bytes == -1) {
    perror("failed to read from file");
    return -1;
  }

  if (lseek(fd, (off_t)0, SEEK_SET) == (off_t)-1) {
    perror("failed seek to start of the file");
    return -1;
  }

  size_t lineno = 0;

  const auto read_input = [&lineno]() {
    (void)printf("enter line no.:\n");

    if (scanf("%lu", &lineno) == EOF) {
      perror("failed to read input");
      return false;
    }

    return true;
  };

  if (not read_input())
    return -1;

  while (lineno != 0) {
    if (lineno <= table.size()) {
      const auto entry = table[lineno - 1];

      std::vector<char> buf{};
      buf.reserve(entry.len + 1);

      if (lseek(fd, entry.offset, SEEK_SET) == (off_t)-1) {
        perror("failed seek to file offset");
        return -1;
      }

      ssize_t read_bytes = read(fd, buf.data(), entry.len * sizeof(char));
      if (read_bytes == -1) {
        perror("failed to read from file");
        return -1;
      }

      if ((size_t)read_bytes != entry.len * sizeof(char)) {
        fprintf(stderr, "failed to read pre-calculated amount of bytes :<\n");
        return -1;
      }

      buf[entry.len] = '\0';

      puts(buf.data());

    } else {
      puts("line out of bounds, try again");
    }

    if (not read_input())
      return -1;
  }

  return 0;
}
