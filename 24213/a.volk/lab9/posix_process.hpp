#include "expected.hpp"
#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#ifndef H_POSIX_PROCESS
#define H_POSIX_PROCESS

struct posix_process_status_t {
  bool signal_terminated = false;
  bool exited = false;
  bool stopped = false;
  bool continued = false;

  int signal;
  int exit_status;
};

class posix_process_t {
private:
  pid_t pid_;
  posix_process_t() = default;

public:
  posix_process_t(const posix_process_t &) = delete;
  posix_process_t &operator=(const posix_process_t &) = delete;

  posix_process_t(posix_process_t &&moved);

  ~posix_process_t();

  static tl::expected<posix_process_t, int>
  fork_spawn(const std::string path,
             const std::vector<std::string> args) noexcept;

  bool terminated() const;

  pid_t get_pid() const;
  std::optional<posix_process_status_t> poll();
  posix_process_status_t wait();
};

#ifdef POSIX_PROCESS_IMPLEMENTATION
// NOLINTBEGIN(misc-definitions-in-headers)

posix_process_t::posix_process_t(posix_process_t &&moved) : pid_(moved.pid_) {
  moved.pid_ = -1;
}

posix_process_t::~posix_process_t() {
  if (this->pid_ == -1)
    return;

  auto res = this->poll();

  if (res.has_value()) {
    if (res.value().exited || res.value().signal_terminated)
      return;
  }

  if (kill(this->pid_, SIGKILL) == -1) {
    perror("Aborting! Error while trying to kill child process");
    std::abort();
  }
}

tl::expected<posix_process_t, int>
posix_process_t::fork_spawn(const std::string path,
                            const std::vector<std::string> args) noexcept {
  pid_t pid = fork();

  std::vector<const char *> cargs;
  cargs.reserve(args.size() + 2);
  cargs.emplace_back(path.c_str());
  for (std::string el : args)
    cargs.emplace_back(el.c_str());
  cargs.push_back(nullptr);

  if (pid == -1)
    return tl::unexpected(errno);

  if (pid == 0) {
    if (execvp(path.c_str(), (char *const *)cargs.data()) == -1) {
      perror("Spawning a new process failed");
      std::abort();
    }
  }

  posix_process_t p;
  p.pid_ = pid;

  return p;
}

pid_t posix_process_t::get_pid() const { return this->pid_; }

bool posix_process_t::terminated() const { return this->pid_ == -1; }

static std::optional<posix_process_status_t> run_waitpid(pid_t pid,
                                                         int options) {
  assert(pid > 0);

  int status;
  pid_t res = waitpid(pid, &status, options);
  if (res == 0)
    return std::nullopt;

  if (res == -1) {
    perror("waitpid failed, which should not happen");
    assert(false && "See error message above");
    std::abort();
  }

  posix_process_status_t s;

  if (WIFEXITED(status)) {
    s.exited = true;
    s.exit_status = WEXITSTATUS(status);
    return s;
  }

  if (WIFSIGNALED(status)) {
    s.signal_terminated = true;
    s.signal = WTERMSIG(status);
    return s;
  }

  if (WIFSTOPPED(status)) {
    s.stopped = true;
    s.signal = WSTOPSIG(status);
    return s;
  }

  assert(false && "Process was polled, but status is not valid");

  return std::nullopt;
}

std::optional<posix_process_status_t> posix_process_t::poll() {
  if (this->terminated())
    return posix_process_status_t{};

  auto res = run_waitpid(this->pid_, WNOHANG);
  if (res.has_value()) {
    if (res.value().exited || res.value().signal_terminated)
      this->pid_ = -1;
  }

  return res;
}

posix_process_status_t posix_process_t::wait() {
  if (this->terminated())
    return posix_process_status_t{};

  auto res = run_waitpid(this->pid_, SA_RESTART);

  if (res.has_value()) {
    if (res.value().exited || res.value().signal_terminated)
      this->pid_ = -1;
    return res.value();
  }

  assert(false && "waitpid did not return valid value");
  std::abort();
}

// NOLINTEND(misc-definitions-in-headers)
#endif

#endif
