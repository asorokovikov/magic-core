#pragma once

#include <fmt/core.h>
#include <fmt/chrono.h>

#include <magic/common/stopwatch.h>
#include <magic/fibers/core/stack.h>

using namespace magic;

bool KeepRunning(Duration duration = 10s, bool reset = false) {
  static Stopwatch sw;
  if (reset) {
    sw.Reset();
  }
  return sw.Elapsed() < duration;
}

void InitializeStressTest() {
  KeepRunning(10s, true);
}


void PrintAllocatorMetrics(const AllocatorMetrics& metrics) {
  fmt::println("Allocator metrics");
  fmt::println("Total allocate: {} ({} KB)", metrics.total_allocate,
               metrics.total_allocate_bytes / (1024));
  fmt::println("AllocateNew: {} times", metrics.allocate_new_count);
  fmt::println("Reused: {} times", metrics.total_allocate - metrics.allocate_new_count);
  fmt::println("Release: {} times", metrics.release_count);
}


struct MoveOnly {
  explicit MoveOnly(std::string value) : data(value) {
  }

  MoveOnly(const MoveOnly& that) = delete;
  MoveOnly& operator=(const MoveOnly& that) = delete;

  MoveOnly(MoveOnly&& that) = default;

  std::string data;
};

struct NonDefaultConstructable {
  explicit NonDefaultConstructable(int v) : value(v) {
  }

  int value{0};
};