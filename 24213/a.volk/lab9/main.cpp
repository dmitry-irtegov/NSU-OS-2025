#include <cassert>
#include <cstring>
#include <iostream>

#define POSIX_PROCESS_IMPLEMENTATION
#include "posix_process.hpp"

int main() {

  auto print_msg_leave = [](int err) {
    std::printf("Failed to launch process: %s", strerror(err));
    std::exit(1);
    return err;
  };

  auto p = posix_process_t::fork_spawn("cat", {"./big_file.txt"})
               .transform_error(print_msg_leave)
               .value();

#ifndef NOWAIT
  posix_process_status_t status = p.wait();
  assert(status.exited);
#endif

  std::cout << p.get_pid() << std::endl;

  return 0;
}
