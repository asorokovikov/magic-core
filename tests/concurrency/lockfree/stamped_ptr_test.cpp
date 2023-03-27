#include <gtest/gtest.h>

#include <magic/concurrency/lockfree/stamped_ptr.h>
#include <magic/concurrency/lockfree/stack.h>

#include <magic/common/random.h>
#include <magic/common/stopwatch.h>

#include <twist/test/race.hpp>
#include <twist/test/lockfree.hpp>
#include <twist/test/plate.hpp>

#include <fmt/core.h>
#include <fmt/format.h>
#include <limits>
#include <thread>

using namespace magic;
using namespace twist::test;

//////////////////////////////////////////////////////////////////////

bool KeepRunning(Duration duration = 10s) {
  static thread_local Stopwatch sw;
  return sw.Elapsed() < duration;
}

//////////////////////////////////////////////////////////////////////

TEST(StampedPtr, JustWorks) {
  uint64_t stamp = 1;
  fmt::println("stamp = {0:064b}", stamp);

  fmt::println("stamp = {0:064b}", (1ul << 16));
  fmt::println("stamp = {0:064b}", (1ul << 16) - 1);
  fmt::println("stamp = {0:064b}", ((1ul << 16) - 1) << 64 - 16);

  auto max_stamp = (1ul << 16) - 1;
  fmt::println("Max stamp: {0:064b}", max_stamp);
  fmt::println("max stamp: {}", max_stamp);

  auto p = new int(42);
  auto ptr = AtomicStampedPtr(p);

  auto stamped_ptr = ptr.Load();
  fmt::println("{}", *stamped_ptr);
  fmt::println("{0:064b}", magic::detail::Packer::Pack(stamped_ptr));

  stamped_ptr = stamped_ptr.IncrementStamp();
  fmt::println("{}", *stamped_ptr);
  fmt::println("{0:064b}", magic::detail::Packer::Pack(stamped_ptr));

  stamped_ptr = stamped_ptr.IncrementStamp();
  fmt::println("{}", *stamped_ptr);
  fmt::println("{0:064b}", magic::detail::Packer::Pack(stamped_ptr));

  stamped_ptr = stamped_ptr.IncrementStamp();
  fmt::println("{}", *stamped_ptr);
  fmt::println("{0:064b}", magic::detail::Packer::Pack(stamped_ptr));
}
