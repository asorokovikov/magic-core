#include <gtest/gtest.h>
#include "../test_helper.h"

#include <magic/fibers/api.h>

#include <magic/fibers/sync/mutex.h>
#include <magic/executors/thread_pool.h>

#include <magic/common/cpu_time.h>
#include <twist/test/plate.hpp>

using Mutex = fibers::Mutex;

//////////////////////////////////////////////////////////////////////

TEST(Mutex, JustWorks) {
  ThreadPool scheduler{4};

  Mutex mutex;

  size_t cs = 0;

  Go(scheduler, [&]() {
    for (size_t j = 0; j < 11; ++j) {
      std::lock_guard guard(mutex);
      ++cs;
    }
  });

  scheduler.WaitIdle();

  ASSERT_EQ(cs, 11);

  scheduler.Stop();
}

TEST(Mutex, Counter) {
  ThreadPool scheduler{4};

  Mutex mutex;

  size_t cs = 0;

  static const size_t kFibers = 10;
  static const size_t kSectionsPerFiber = 1024;

  for (size_t i = 0; i < kFibers; ++i) {
    Go(scheduler, [&]() {
      for (size_t j = 0; j < kSectionsPerFiber; ++j) {
        std::lock_guard guard(mutex);
        ++cs;
      }
    });
  }

  scheduler.WaitIdle();

  std::cout << "# cs = " << cs << std::endl;

  ASSERT_EQ(cs, kFibers * kSectionsPerFiber);

  scheduler.Stop();
}

TEST(Mutex, Blocking) {
  ThreadPool scheduler{4};

  Mutex mutex;

  ProcessCPUTimer timer;

  Go(scheduler, [&]() {
    mutex.Lock();
    std::this_thread::sleep_for(1s);
    mutex.Unlock();
  });

  Go(scheduler, [&]() {
    mutex.Lock();
    mutex.Unlock();
  });

  scheduler.WaitIdle();

  ASSERT_TRUE(timer.Elapsed() < 100ms);

  scheduler.Stop();
}

TEST(Mutex, Extra) {
  ThreadPool scheduler{4};

  Mutex mutex;

  Go(scheduler, [&]() {
    mutex.Lock();
    std::this_thread::sleep_for(1s);
    mutex.Unlock();
  });

  std::this_thread::sleep_for(128ms);

  for (size_t i = 0; i < 3; ++i) {
    Go(scheduler, [&]() {
      mutex.Lock();
      mutex.Unlock();
      std::this_thread::sleep_for(5s);
    });
  }

  scheduler.WaitIdle();
  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

void StressTest1(size_t fibers, Duration duration) {
  InitializeStressTest();

  ThreadPool scheduler{4};

  fibers::Mutex mutex;
  twist::test::Plate plate;

  for (size_t i = 0; i < fibers; ++i) {
    Go(scheduler, [&]() {
      while (KeepRunning(duration)) {
        mutex.Lock();
        plate.Access();
        mutex.Unlock();
      }
    });
  }

  scheduler.WaitIdle();

  std::cout << "# critical sections: " << plate.AccessCount() << std::endl;
  ASSERT_TRUE(plate.AccessCount() > 12345);

  scheduler.Stop();
}

TEST(Mutex, StressTest_1) {
  StressTest1(4, 5s);
}

TEST(Mutex, StressTest_1_2) {
  StressTest1(16, 5s);
}

TEST(Mutex, StressTest_1_3) {
  StressTest1(100, 5s);
}

//////////////////////////////////////////////////////////////////////

void StressTest2(size_t fibers, Duration duration) {
  InitializeStressTest();

  ThreadPool scheduler{4};

  while (KeepRunning(duration)) {
    fibers::Mutex mutex;
    std::atomic<size_t> cs{0};

    for (size_t j = 0; j < fibers; ++j) {
      Go(scheduler, [&]() {
        mutex.Lock();
        ++cs;
        mutex.Unlock();
      });
    }

    scheduler.WaitIdle();

    ASSERT_EQ(cs, fibers);
  }

  scheduler.Stop();
}

TEST(Mutex, StressTest_2_1) {
  StressTest2(2, 5s);
}

TEST(Mutex, StressTest_2_2) {
  StressTest2(3, 5s);
}

//////////////////////////////////////////////////////////////////////


