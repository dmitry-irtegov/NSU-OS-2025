#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

#ifndef TIME_OUT
#define TIME_OUT 5
#endif

static inline std::runtime_error errno_runtime_error() {
  return std::runtime_error(strerror(errno));
}

struct fd_handler_t {
  int fd;
  bool is_canonical_term;

  fd_handler_t(int fd, bool is_canonical_term)
      : fd{fd}, is_canonical_term(is_canonical_term) {}
 
  std::optional<std::string> readline() {
    return this->is_canonical_term ? readline_buf(BUFFER_SIZE)
                                   : readline_buf(1);
  }

  bool wait_for_fd() {
    fd_set s;
    FD_ZERO(&s);
    FD_SET(fd, &s);
    struct timeval tv;
    tv.tv_sec = TIME_OUT;
    tv.tv_usec = 0;

    int sel = select(fd + 1, &s, NULL, NULL, &tv);

    if (sel == -1)
      throw errno_runtime_error();

    return FD_ISSET(fd, &s);
  }

private:
  std::optional<std::string> readline_buf(size_t buffsz) {
    std::string line{};
    auto buf = std::make_unique<char[]>(buffsz);

    ssize_t r;

    do {
      if ((r = read(fd, buf.get(), buffsz)) == -1)
        throw errno_runtime_error();

      if (r != 0)
        line.append(buf.get(), buf.get() + r);

    } while (r != 0 && buf[r - 1] != '\n');

    return line.empty() ? std::nullopt : std::optional(line);
  }
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << argv[0] << " file..." << std::endl;
    return 1;
  }

  std::vector<fd_handler_t> fds{};

  for (int i = 1; i < argc; i++) {
    int fd = open(argv[i], O_RDONLY);
    if (fd == -1)
      throw errno_runtime_error();

    bool is_canonical_term = false;
    if (isatty(fd)) {
      struct termios tio;
      if (tcgetattr(fd, &tio) == -1)
        throw errno_runtime_error();
      is_canonical_term = tio.c_lflag & ICANON;
    }

    fds.emplace_back(fd, is_canonical_term);
  }

  while (!fds.empty()) {
    size_t i = 0;
    while (i < fds.size()) {
      if (fds[i].wait_for_fd()) {
        auto line = fds[i].readline();
        if (line.has_value()) {
          std::cout << line.value() << std::flush;
        } else {
          // no value means EOF
          fds.erase(fds.cbegin() + i);
          continue;
        }
      }
      i++;
    }
  }

  return 0;
}
