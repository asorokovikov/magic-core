#include <gtest/gtest.h>

#include "../../../test_helper.h"

#include <magic/coroutine/stackless/dispatch.h>
#include <magic/coroutine/stackless/yield.h>
#include <magic/coroutine/stackless/fire.h>
#include <magic/coroutine/stackless/sync/mutex.h>

#include <magic/executors/manual.h>
#include <magic/executors/thread_pool.h>

#include <twist/test/plate.hpp>

using namespace magic;

//////////////////////////////////////////////////////////////////////

TEST(Stackless_Mutex, JustWorks) {
  Mutex mutex;

  ASSERT_TRUE(mutex.TryLock());
  mutex.Unlock();
}

TEST(Stackless_Mutex, Yield) {
  ManualExecutor scheduler;

  Mutex mutex;
  size_t counter = 0;

  static const size_t kSections = 3;
  static const size_t kContenders = 4;
  static const size_t kYields = 5;

  auto contender = [&]() -> Task<> {
    co_await DispatchTo(scheduler);

    for (size_t i = 0; i < kSections; ++i) {
      auto lock = co_await mutex.ScopedLock();

      assert(counter++ == 0);

      for (size_t k = 0; k < kYields; ++k) {
        co_await Yield(scheduler);
      }

      --counter;
    }
  };

  for (size_t j = 0; j < kContenders; ++j) {
    FireAndForget(contender());
  }

  size_t steps = scheduler.RunAll();
  fmt::println("steps {}, expected greater than {}", steps, kContenders * kSections * kYields);

  ASSERT_GE(steps, kContenders * kSections * kYields);
}

TEST(Stackless_Mutex, ThreadPool) {
  ThreadPool scheduler{4};

  Mutex mutex;
  size_t cs = 0;

  static const size_t kSections = 123456;
  static const size_t kContenders = 17;

  auto contender = [&]() -> Task<> {
    co_await DispatchTo(scheduler);

    for (size_t i = 0; i < kSections; ++i) {
      auto lock = co_await mutex.ScopedLock();
      ++cs;
    }
  };

  for (size_t j = 0; j < kContenders; ++j) {
    FireAndForget(contender());
  }

  scheduler.WaitIdle();

  ASSERT_EQ(cs, kContenders * kSections);

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

void StressTest1(size_t fibers, Duration duration) {
  InitializeStressTest();

  ThreadPool scheduler{4};

  Mutex mutex;
  twist::test::Plate plate;

  auto contender = [&]() -> Task<> {
    co_await DispatchTo(scheduler);

    size_t iter = 0;
    while (KeepRunning(duration)) {
      auto lock = co_await mutex.ScopedLock();

      plate.Access();
      if (++iter % 7 == 0) {
        co_await Yield(scheduler);
      }
    }
  };

  for (size_t i = 0; i < fibers; ++i) {
    FireAndForget(contender());
  }

  scheduler.WaitIdle();

  std::cout << "# critical sections: " << plate.AccessCount() << std::endl;
  ASSERT_TRUE(plate.AccessCount() > 100500);

  scheduler.Stop();
}

TEST(Stackless_Mutex, Stress_1_1) {
  StressTest1(4, 10s);
}

TEST(Stackless_Mutex, Stress_1_2) {
  StressTest1(16, 5s);
}

TEST(Stackless_Mutex, Stress_1_3) {
  StressTest1(100, 5s);
}

//////////////////////////////////////////////////////////////////////

void StressTest2(size_t gorroutines, Duration duration) {
  InitializeStressTest();

  ThreadPool scheduler{4};

  while (KeepRunning(duration)) {
    Mutex mutex;
    size_t cs = 0;

    auto contender = [&]() -> Task<> {
      co_await DispatchTo(scheduler);

      {
        auto lock = co_await mutex.ScopedLock();
        ++cs;
      }
    };

    for (size_t j = 0; j < gorroutines; ++j) {
      FireAndForget(contender());
    }

    scheduler.WaitIdle();

    ASSERT_EQ(cs, gorroutines);
  }

  scheduler.Stop();
}

TEST(Stackless_Mutex, Stress_2_1) {
  StressTest2(2, 10s);
}

TEST(Stackless_Mutex, Stress_2_2) {
  StressTest2(3, 10s);
}

