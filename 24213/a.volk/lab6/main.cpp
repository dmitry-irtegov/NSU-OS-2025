#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define BUFFER_SIZE 4096

void usage(const char *const invoke_path) {
  printf("%s <text file>\n", invoke_path);
}

struct fatal_error_t : public std::exception {

  const char *what() const noexcept {
    if (m_cstr_msg)
      return m_cstr_msg;

    return m_msg.c_str();
  }

  fatal_error_t(std::string msg) : m_msg{msg} {}
  fatal_error_t(const char *const cstr_msg) : m_cstr_msg{cstr_msg} {}

private:
  const char *const m_cstr_msg = nullptr;
  std::string m_msg;
};

template <size_t buffer_size_ = BUFFER_SIZE> struct file_reader_t {

  static const size_t buffer_size = buffer_size_;

  using buffer = std::array<char, buffer_size>;

  file_reader_t(int fd, off_t init_pos) : m_fd{fd}, m_buf{}, m_in_buf(0) {
    if (lseek(fd, init_pos, SEEK_SET) == static_cast<off_t>(-1))
      throw fatal_error_t{strerror(errno)};
  }

  file_reader_t(int fd) : file_reader_t{fd, static_cast<off_t>(0)} {}

  size_t load_buffer(off_t to_read) {
    ssize_t s = read(m_fd, m_buf.data(),
                     to_read < static_cast<off_t>(buffer_size)
                         ? static_cast<size_t>(to_read)
                         : buffer_size);
    if (s == static_cast<ssize_t>(-1))
      throw fatal_error_t{strerror(errno)};

    m_in_buf = static_cast<size_t>(s);

    return m_in_buf;
  }

  size_t load_buffer(void) {
    return load_buffer(static_cast<off_t>(buffer_size));
  }

  const buffer &get_buffer() { return m_buf; }

  auto in_buffer() { return m_in_buf; }

private:
  int m_fd;
  buffer m_buf;
  typename buffer::difference_type m_in_buf;
};

struct table_entry_t {
  off_t offset;
  off_t len;
};

using table_t = std::vector<table_entry_t>;

table_t create_table(int fd) {
  table_t table{};

  file_reader_t reader{fd};

  off_t prev_off = -1;
  off_t curr_off = -1;

  while (reader.load_buffer() != 0) {
    const auto &buf = reader.get_buffer();
    for (auto it = buf.begin(); it != (buf.begin() + reader.in_buffer());
         ++it) {
      curr_off++;
      if (*it == '\n') {
        table.emplace_back(table_entry_t{prev_off + 1, (curr_off - prev_off)});
        prev_off = curr_off;
      }
    }
  }

  return table;
}

ssize_t read_line_no() {
  fd_set s;
  FD_ZERO(&s);
  FD_SET(STDIN_FILENO, &s);
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  std::cout << "you have 5 seconds to enter line no." << std::endl;

  size_t no = 0;
  int sel;

  while ((std::cout << "line no.: " << std::endl) &&
         (sel = select(1, &s, NULL, NULL, &tv)) == 1) {

    if (!(std::cin >> no)) {
      if (std::cin.bad())
        throw fatal_error_t{"input is corrupted!"};

      if (std::cin.eof())
        return 0;

      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    } else {
      return no;
    }

    FD_ZERO(&s);
    FD_SET(STDIN_FILENO, &s);
    tv.tv_sec = 5;
    tv.tv_usec = 0;
  }

  if (sel == -1)
    throw fatal_error_t{strerror(errno)};

  assert(sel == 0);

  return -1;
}

void do_thing(int fd) {
  table_t table = create_table(fd);

  ssize_t line_no = 0;
  while ((line_no = read_line_no()) > 0) {
    if (static_cast<table_t::size_type>(line_no) > table.size()) {
      std::cout << "line no. out of bounds" << std::endl;
      continue;
    }

    file_reader_t reader{fd, table[line_no - 1].offset};
    off_t rest = table[line_no - 1].len;

    while (rest > static_cast<off_t>(0)) {
      rest -= static_cast<off_t>(reader.load_buffer(rest));
      const auto &buf = reader.get_buffer();
      std::copy(buf.begin(), buf.begin() + reader.in_buffer(),
                std::ostream_iterator<char>{std::cout});
    }
  }

  if (line_no == -1) {
    file_reader_t reader{fd};

    while (reader.load_buffer() != 0) {
      const auto &buf = reader.get_buffer();
      std::copy(buf.begin(), buf.begin() + reader.in_buffer(),
                std::ostream_iterator<char>{std::cout});
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    usage(argv[0]);
    return -1;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    perror("failed to open file");
    return -1;
  }

  const auto file_close = [](int *fd) { close(reinterpret_cast<int64_t>(fd)); };
  const std::unique_ptr<int, decltype(file_close)> _fd_defer_close{
      reinterpret_cast<int *>(fd), file_close};

  try {
    do_thing(fd);
  } catch (std::exception &e) {
    fprintf(stderr, "failed: %s\n", e.what());
    return 1;
  }

  return 0;
}
