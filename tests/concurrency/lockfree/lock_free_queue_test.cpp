#include <gtest/gtest.h>

#include <magic/common/random.h>
#include <magic/common/stopwatch.h>

#include <magic/concurrency/barrier.h>
#include <magic/concurrency/lockfree/queue.h>

#include <twist/test/race.hpp>
#include <twist/test/lockfree.hpp>

#include <fmt/core.h>
#include <limits>
#include <thread>

using namespace magic;
using namespace twist::test;

//////////////////////////////////////////////////////////////////////

bool KeepRunning(Duration duration = 10s) {
  static thread_local Stopwatch sw;
  return sw.Elapsed() < duration;
}

struct MoveOnly {
  explicit MoveOnly(int _value) : value(_value) {
  }

  // Non-movable
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;

  // Movable
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;

  int value;
};

//////////////////////////////////////////////////////////////////////

TEST(LockFreeQueue, JustWorks) {
  LockFreeQueue<int> queue;
  queue.Put(42);
  ASSERT_EQ(queue.Take(), 42);
  ASSERT_FALSE(queue.Take());
}

TEST(LockFreeQueue, MoveOnly) {
  LockFreeQueue<MoveOnly> queue;
  queue.Put(MoveOnly{1});
  auto item = queue.Take();
}

TEST(LockFreeQueue, FifoSmall) {
  LockFreeQueue<int> queue;
  queue.Put(1);
  queue.Put(2);
  queue.Put(3);

  ASSERT_EQ(queue.Take(), 1);
  ASSERT_EQ(queue.Take(), 2);
  ASSERT_EQ(queue.Take(), 3);

  ASSERT_FALSE(queue.Take());
}

TEST(LockFreeQueue, FifoSmall_TakeAll) {
  LockFreeQueue<int> queue;
  queue.Put(1);
  queue.Put(2);
  queue.Put(3);

  auto items = queue.TakeAll();
  ASSERT_EQ(items.size(), 3);
  ASSERT_EQ(items.front(), 1);
  items.pop_front();
  ASSERT_EQ(items.front(), 2);
  items.pop_front();
  ASSERT_EQ(items.front(), 3);

  ASSERT_TRUE(queue.IsEmpty());
  ASSERT_FALSE(queue.Take());
}

TEST(LockFreeQueue, FifoLarge) {
  static const size_t kMaxItems = 12345;
  LockFreeQueue<int> queue;

  auto barrier = CyclicBarrier(2);

  // Producer

  std::thread producer([&] {
    barrier.ArriveAndWait();
    for (size_t index = 0; index < kMaxItems; ++index) {
      queue.Put(index);
    }
  });

  // Consumer

  std::thread consumer([&] {
    barrier.ArriveAndWait();
    for (size_t index = 0; index < kMaxItems; ++index) {
      auto value = queue.Take();
      if (!value) {
        --index;
        continue;
      }
      ASSERT_EQ(value, index);
    }
  });

  producer.join();
  consumer.join();
}