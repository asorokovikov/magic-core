#include <gtest/gtest.h>

#include "../test_helper.h"

#include <magic/fibers/api.h>

//////////////////////////////////////////////////////////////////////

TEST(Fibers, JustWorks) {
  RunScheduler(1, [] {
    self::Yield();
  });

  PrintAllocatorMetrics(GetAllocatorMetrics());
}

//////////////////////////////////////////////////////////////////////

TEST(Fibers, TwoFibers) {
  RunScheduler(1, [] {
    size_t step = 0;
    Go([&]() {
      for (size_t index = 1; index < 7; ++index) {
        step = index;
        self::Yield();
      }
    });

    for (size_t index = 1; index < 7; ++index) {
      self::Yield();
      ASSERT_EQ(step, index);
    }
  });

  PrintAllocatorMetrics(GetAllocatorMetrics());
}

//////////////////////////////////////////////////////////////////////

TEST(Fibers, Yield) {
  std::atomic<int> value = 0;

  auto check_value = [&] {
    const int kLimit = 10;

    ASSERT_TRUE(value.load() < kLimit);
    ASSERT_TRUE(value.load() > -kLimit);
  };

  static const size_t kIterations = 12345;

  auto bull = [&] {
    for (size_t index = 0; index < kIterations; ++index) {
      value.fetch_add(1);
      self::Yield();
      check_value();
    }
  };

  auto bear = [&] {
    for (size_t index = 0; index < kIterations; ++index) {
      value.fetch_sub(1);
      self::Yield();
      check_value();
    }
  };

  RunScheduler(1, [&] {
    Go(bull);
    Go(bear);
  });

  PrintAllocatorMetrics(GetAllocatorMetrics());
}

//////////////////////////////////////////////////////////////////////

TEST(Fibers, Yield2) {
  static const size_t kYields = 65536;

  auto tester = []() {
    for (size_t index = 0; index < kYields; ++index) {
      self::Yield();
    }
  };

  RunScheduler(4, [&] {
    Go(tester);
    Go(tester);
  });

  PrintAllocatorMetrics(GetAllocatorMetrics());
}

//////////////////////////////////////////////////////////////////////

TEST(Fibers, Stress) {
  RunScheduler(4, [] {
    for (size_t index = 0; index < 127; ++index) {
      Go([] {
        for (size_t index = 0; index < 12345; ++index) {
          self::Yield();
        }
      });
    }
  });

  PrintAllocatorMetrics(GetAllocatorMetrics());
}