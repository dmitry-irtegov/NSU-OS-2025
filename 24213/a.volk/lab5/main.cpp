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
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define FILE_READER_ITERATOR_CHECK

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

class buffered_file_reader_t {
  static constexpr size_t buffer_size = 4096;
  using buffer_type = std::array<char, buffer_size>;

  const int m_fd;

  ssize_t load_buffer() {
    ssize_t s = read(m_fd, m_buffer.data(), buffer_size);
    if (s == static_cast<ssize_t>(-1))
      throw fatal_error_t{strerror(errno)};

    m_in_buffer = static_cast<size_t>(s);

    return s;
  }

  struct input_iterator_t {
    friend class buffered_file_reader_t;

    using iterator_category = std::input_iterator_tag;
    using difference_type = off_t;
    using value_type = char;
    using pointer = buffer_type::iterator;
    using reference = char &;

    reference operator*() const { return *m_current; }
    pointer operator->() { return m_current; }

    input_iterator_t &operator++() { // todo
      if (m_current == m_parent.m_buffer.end())
        return *this;

      if ((m_pos++) == m_parent.m_to_read) {
        m_current = m_parent.m_buffer.end();
        return *this;
      }

      const size_t buf_pos = static_cast<size_t>(
          std::distance(m_parent.m_buffer.begin(), m_current));

      if (buf_pos == m_parent.m_in_buffer - 1) {
        ssize_t s = m_parent.load_buffer();
        if (s == 0) {
          m_current = m_parent.m_buffer.end();
          return *this;
        }

        m_current = m_parent.m_buffer.begin();

        return *this;
      }

      ++m_current;

      return *this;
    }

    input_iterator_t operator++(int) {
      const auto &tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const input_iterator_t &lhs,
                           const input_iterator_t &rhs) {
      return lhs.m_current == rhs.m_current;
    };

    friend bool operator!=(const input_iterator_t &lhs,
                           const input_iterator_t &rhs) {
      return !(lhs == rhs);
    };

  private:
    buffered_file_reader_t &m_parent;
    buffer_type::iterator m_current;
    off_t m_pos;

    input_iterator_t(buffered_file_reader_t &parent, off_t pos)
        : m_parent{parent}, m_pos(pos) {}
  };

public:
  using iterator = input_iterator_t;

  buffered_file_reader_t(int fd, off_t start_pos, off_t to_read)
      : m_fd{fd}, m_to_read{to_read}, m_in_buffer(0) {
    if (lseek(fd, start_pos, SEEK_SET) == static_cast<off_t>(-1))
      throw fatal_error_t(strerror(errno));
  }

  buffered_file_reader_t(int fd, off_t start_pos)
      : buffered_file_reader_t{fd, start_pos,
                               std::numeric_limits<off_t>::max()} {}

  buffered_file_reader_t(int fd) : buffered_file_reader_t{fd, 0} {}

  iterator begin() {
#ifdef FILE_READER_ITERATOR_CHECK
    assert(!m_began_);
    m_began_ = true;
#endif

    load_buffer();
    iterator i{*this, 0};
    i.m_current = m_buffer.begin();

    return i;
  }

  iterator end() {
    iterator i{*this, m_to_read};
    i.m_current = m_buffer.end();
    return i;
  }

private:
  off_t m_to_read;
  size_t m_in_buffer;
  buffer_type m_buffer;
#ifdef FILE_READER_ITERATOR_CHECK
  bool m_began_ = false;
#endif
};

struct table_entry_t {
  off_t offset;
  off_t len;
};

using table_t = std::vector<table_entry_t>;

table_t create_table(int fd) {
  table_t table{};
  buffered_file_reader_t reader{fd};

  off_t prev_off = -1;
  off_t curr_off = -1;

  for (const char c : reader) {
    curr_off++;
    if (c == '\n') {
      table.emplace_back(table_entry_t{prev_off + 1, (curr_off - prev_off)});
      prev_off = curr_off;
    }
  }

  return table;
}

void do_thing(int fd) {

  table_t table = create_table(fd);

  size_t lineno = 0;

  const auto read_input = [&lineno]() {
    (void)printf("enter line no.:\n");

    if (scanf("%lu", &lineno) == EOF)
      return false;

    return true;
  };

  if (!read_input())
    return;

  while (lineno != 0) {
    if (lineno <= table.size()) {
      const auto entry = table[lineno - 1];

      buffered_file_reader_t reader{fd, entry.offset, entry.len};
      std::copy(reader.begin(), reader.end(),
                std::ostream_iterator<char>{std::cout});

    } else {
      puts("line out of bounds, try again");
    }

    if (!read_input())
      return;
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
  }

  return 0;
}
