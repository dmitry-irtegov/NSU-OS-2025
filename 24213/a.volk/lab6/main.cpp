#include <cerrno>
#include <cstdint>
#include <cstdio>
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
#include <sys/select.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>
#include <vector>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE BUFSIZ
#endif

using table_t = std::vector<std::tuple<off_t, off_t>>;

size_t read_throwing(int fd, char buf[BUFFER_SIZE],
                     off_t to_read = BUFFER_SIZE) {
  size_t read_size = (size_t)std::min(to_read, (off_t)BUFFER_SIZE);

  ssize_t s = read(fd, buf, read_size);
  if (s == -1)
    throw std::runtime_error{strerror(errno)};

  return (size_t)s;
}

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

table_t create_table(int fd) {
  table_t table{};

  if (lseek(fd, 0, SEEK_SET) == -1)
    throw std::runtime_error{strerror(errno)};

  off_t prev_off = -1;
  off_t curr_off = -1;

  char buf[BUFFER_SIZE];
  size_t in_buf = 0;

  while ((in_buf = read_throwing(fd, buf)) != 0) {
    for (size_t i = 0; i < in_buf; i++) {
      curr_off++;
      if (buf[i] == '\n') {
        table.emplace_back(prev_off + 1, curr_off - prev_off);
        prev_off = curr_off;
      }
    }
  }

  return table;
}

void read_file_to(int fd, const std::ostream_iterator<char> &out,
                  off_t limit = std::numeric_limits<off_t>::max()) {
  char buf[BUFFER_SIZE];

  off_t rest = limit;
  size_t in_buf = 0;

  while (rest > (off_t)0) {
    in_buf = read_throwing(fd, buf, rest);
    rest -= (off_t)in_buf;

    if (in_buf == 0)
      return; // eof achieved

    std::copy(buf, buf + in_buf, out);
  }
}

void do_thing(const char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1)
    throw std::runtime_error{strerror(errno)};

  const auto file_close = [](int *fd) {
    close(reinterpret_cast<intptr_t>(fd));
  };
  const std::unique_ptr<int, decltype(file_close)> _fd_defer_close{
      reinterpret_cast<int *>(fd), file_close};

  table_t table = create_table(fd);

  ssize_t line_no = 0;
  while ((line_no = read_line_no()) > 0) {
    if (line_no > (ssize_t)table.size()) {
      std::cout << "line no. out of bounds" << std::endl;
      continue;
    }

    if (lseek(fd, std::get<0>(table[line_no - 1]), SEEK_SET) == -1)
      throw std::runtime_error{strerror(errno)};

    read_file_to(fd, std::ostream_iterator<char>{std::cout},
                 std::get<1>(table[line_no - 1]));
  }

  if (line_no == -1) {
    if (lseek(fd, 0, SEEK_SET) == -1)
      throw std::runtime_error{strerror(errno)};

    read_file_to(fd, std::ostream_iterator<char>{std::cout});
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
