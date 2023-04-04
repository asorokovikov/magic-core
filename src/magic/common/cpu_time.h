#pragma once

#include <ctime>
#include <cstdlib>
#include <chrono>

namespace magic {

//////////////////////////////////////////////////////////////////////

class ThreadCPUTimer final {
 public:
  ThreadCPUTimer() {
    Reset();
  }

  auto Elapsed() const -> std::chrono::nanoseconds {
    return std::chrono::nanoseconds(ElapsedNanos());
  }

  void Reset() {
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_);
  }

 private:
  auto ElapsedNanos() const -> uint64_t {
    struct timespec now;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now);
    return ToNanos(now) - ToNanos(start_);
  }

  static uint64_t ToNanos(const struct timespec& tp) {
    return tp.tv_sec * 1'000'000'000 + tp.tv_nsec;
  }

  struct timespec start_;
};

//////////////////////////////////////////////////////////////////////

class ProcessCPUTimer {
 public:
  ProcessCPUTimer() {
    Reset();
  }

  std::chrono::microseconds Elapsed() const {
    return std::chrono::microseconds(ElapsedMicros());
  }

  void Reset() {
    start_ts_ = std::clock();
  }

 private:
  size_t ElapsedMicros() const {
    const size_t clocks = std::clock() - start_ts_;
    return ClocksToMicros(clocks);
  }

  static size_t ClocksToMicros(const size_t clocks) {
    return (clocks * 1'000'000) / CLOCKS_PER_SEC;
  }

 private:
  std::clock_t start_ts_;
};

//////////////////////////////////////////////////////////////////////

class CpuTimeBudgetGuard {
 public:
  explicit CpuTimeBudgetGuard(std::chrono::milliseconds budget)
      : budget_(budget) {}

  std::chrono::microseconds Usage() const {
    return timer_.Elapsed();
  }

  ~CpuTimeBudgetGuard() {
    assert(timer_.Elapsed() < budget_);
  }

 private:
  std::chrono::milliseconds budget_;
  ProcessCPUTimer timer_;
};


//////////////////////////////////////////////////////////////////////

}  // namespace magic