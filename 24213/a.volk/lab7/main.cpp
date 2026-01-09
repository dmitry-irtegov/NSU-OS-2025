#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>
#include <vector>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE BUFSIZ
#endif

template <typename Deleter> using defer = std::unique_ptr<void, Deleter>;

using table_t = std::vector<std::tuple<off_t, off_t>>;

bool wait_for_stdin() {
  fd_set s;
  FD_ZERO(&s);
  FD_SET(STDIN_FILENO, &s);
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  int sel = select(1, &s, NULL, NULL, &tv);

  if (sel == -1)
    throw std::runtime_error{strerror(errno)};

  return FD_ISSET(STDIN_FILENO, &s);
}

ssize_t read_line_no() {
  while ((std::cout << "you have 5 seconds to enter line no.\n"
                    << "line no.: " << std::flush) &&
         wait_for_stdin()) {
    bool failed = false;
    ssize_t no;

    if (!(std::cin >> no)) {
      failed = true;
      std::cin.clear();
    }

    std::string line{};
    std::getline(std::cin, line);

    if (std::cin.eof())
      return 0;

    if (!line.empty())
      std::cerr << "[WARNING] input ignored: " << line << std::endl;

    if (failed)
      continue;

    if (no <= 0) {
      std::cout << "line no. must be > 0" << std::endl;
      continue;
    }

    return (ssize_t)no;
  }

  return -1;
}

table_t create_table(char *mapped, off_t size) {
  table_t table{};

  off_t prev_off = -1;
  off_t curr_off = -1;

  for (off_t i = 0; i < size; i++) {
    curr_off++;
    if (mapped[i] == '\n') {
      table.emplace_back(prev_off + 1, curr_off - prev_off);
      prev_off = curr_off;
    }
  }

  return table;
}

void do_thing(const char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1)
    throw std::runtime_error{strerror(errno)};

  const auto fd_close = [fd](void *_p __attribute__((unused))) { close(fd); };
  defer<decltype(fd_close)> _close{nullptr, fd_close};

  struct stat sb;
  if (fstat(fd, &sb) == -1)
    throw std::runtime_error{strerror(errno)};

  off_t file_sz = sb.st_size;
  if (file_sz > std::numeric_limits<ssize_t>::max())
    throw std::runtime_error{"cannot map large file"};

  char *mapped =
      (char *)mmap(NULL, (size_t)file_sz, PROT_READ, MAP_PRIVATE, fd, 0);
  if ((void *)mapped == MAP_FAILED)
    throw std::runtime_error{strerror(errno)};

  const auto unmap = [mapped, file_sz](void *_p __attribute__((unused))) {
    munmap(mapped, file_sz);
  };
  defer<decltype(unmap)> _unmap{nullptr, unmap};

  table_t table = create_table(mapped, file_sz);

  const auto out_it = std::ostream_iterator<char>{std::cout};

  ssize_t line_no = 0;
  while ((line_no = read_line_no()) > 0) {
    if (line_no > (ssize_t)table.size()) {
      std::cout << "line no. out of bounds" << std::endl;
      continue;
    }

    off_t off = std::get<0>(table[line_no - 1]);
    off_t line_sz = std::get<1>(table[line_no - 1]);

    std::copy(mapped + off, mapped + off + line_sz, out_it);
  }

  if (line_no == -1) {
    std::cout << std::endl;
    std::copy(mapped, mapped + file_sz, out_it);
  }
}

int main(int argc, char *argv[]) {
  std::setbuf(stdin, nullptr);
  std::ios_base::sync_with_stdio(true);
  std::cin.exceptions(std::istream::badbit);

  if (argc != 2) {
    // print usage
    std::cerr << argv[0] << " <text file>" << std::endl;
    return 1;
  }

  try {
    do_thing(argv[1]);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
