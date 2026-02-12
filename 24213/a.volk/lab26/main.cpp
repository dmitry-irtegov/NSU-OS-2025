#include <cstdio>
#include <cstring>
#include <exception>
#include <functional>
#include <ios>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE BUFSIZ
#endif

void run() {
  FILE *outpipe = popen("tr a-z A-Z", "w");
  if (outpipe == nullptr)
    throw std::runtime_error{strerror(errno)};
  std::unique_ptr<FILE, std::function<int(FILE *)>> u{outpipe, pclose};

  char buf[BUFFER_SIZE];

  while (!std::cin.eof()) {
    std::cin.read(buf, BUFFER_SIZE - 1);
    buf[std::cin.gcount()] = '\0';
    if (fputs(buf, outpipe) == EOF)
      throw std::runtime_error{strerror(errno)};
  }
}

int main() {
  std::cin.exceptions(std::ios::badbit);

  try {
    run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
