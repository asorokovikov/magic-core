#include <gtest/gtest.h>

#include "../test_helper.h"

#include <magic/common/cpu_time.h>
#include <magic/fibers/sync/wait_group.h>

using WaitGroup = magic::fibers::WaitGroup;

TEST(WaitGroup, JustWorks) {
  ThreadPool scheduler{4};

  WaitGroup wg;
  wg.Add();

  std::atomic<bool> done = false;

  Go(scheduler, [&] {
    wg.Wait();
    ASSERT_FALSE(done.load());
    done = true;
  });

  wg.Done();

  scheduler.WaitIdle();
  ASSERT_TRUE(done.load());
  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(WaitGroup, OneWaiter) {
  ThreadPool scheduler{5};

  WaitGroup wg;

  std::atomic<size_t> workers{0};
  std::atomic<bool> waiter{false};

  static const size_t kWorkers = 3;

  wg.Add(kWorkers);

  Go(scheduler, [&]() {
    wg.Wait();
    ASSERT_EQ(workers.load(), kWorkers);
    waiter = true;
  });

  for (size_t i = 0; i < kWorkers; ++i) {
    Go(scheduler, [&]() {
      std::this_thread::sleep_for(1s);
      ++workers;
      wg.Done();
    });
  }

  scheduler.WaitIdle();

  ASSERT_TRUE(waiter);

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(WaitGroup, MultipleWaiters) {
  ThreadPool scheduler{5};

  WaitGroup wg;

  std::atomic<size_t> workers{0};
  std::atomic<size_t> waiters{0};

  static const size_t kWorkers = 3;
  static const size_t kWaiters = 4;

  wg.Add(kWorkers);

  for (size_t i = 0; i < kWaiters; ++i) {
    Go(scheduler, [&]() {
      wg.Wait();
      ASSERT_EQ(workers.load(), kWorkers);
      ++waiters;
    });
  }

  for (size_t i = 0; i < kWorkers; ++i) {
    Go(scheduler, [&]() {
      std::this_thread::sleep_for(1s);
      ++workers;
      wg.Done();
    });
  }

  scheduler.WaitIdle();

  ASSERT_EQ(waiters, kWaiters);

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(WaitGroup, BlockingWait) {
  ThreadPool scheduler{4};

  WaitGroup wg;

  std::atomic<size_t> workers = 0;

  ProcessCPUTimer timer;

  static const size_t kWorkers = 3;

  wg.Add(kWorkers);

  Go(scheduler, [&]() {
    wg.Wait();
    ASSERT_EQ(workers.load(), kWorkers);
  });

  for (size_t i = 0; i < kWorkers; ++i) {
    Go(scheduler, [&]() {
      std::this_thread::sleep_for(1s);
      ++workers;
      wg.Done();
    });
  }

  scheduler.WaitIdle();

  ASSERT_TRUE(timer.Elapsed() < 100ms);

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

void StressTest1(size_t workers, size_t waiters, Duration duration) {
  InitializeStressTest();

  ThreadPool scheduler{/*threads=*/4};

  while (KeepRunning(duration)) {
    fibers::WaitGroup wg;

    std::atomic<size_t> waiters_done{0};
    std::atomic<size_t> workers_done{0};

    wg.Add(workers);

    // Waiters

    for (size_t i = 0; i < waiters; ++i) {
      Go(scheduler, [&]() {
        wg.Wait();
        waiters_done.fetch_add(1);
      });
    }

    // Workers

    for (size_t j = 0; j < workers; ++j) {
      Go(scheduler, [&]() {
        workers_done.fetch_add(1);
        wg.Done();
      });
    }

    scheduler.WaitIdle();

    ASSERT_EQ(waiters_done.load(), waiters);
    ASSERT_EQ(workers_done.load(), workers);
  }

  scheduler.Stop();
}

TEST(WaitGroup, Stress_1_1) {
  StressTest1(1, 1, 5s);
}

TEST(WaitGroup, Stress_1_2) {
  StressTest1(2, 2, 5s);
}

TEST(WaitGroup, Stress_1_3) {
  StressTest1(3, 1, 5s);
}