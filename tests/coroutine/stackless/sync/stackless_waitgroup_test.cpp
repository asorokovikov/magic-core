#include <gtest/gtest.h>

#include "../../../test_helper.h"

#include <magic/common/cpu_time.h>

#include <magic/coroutine/stackless/dispatch.h>
#include <magic/coroutine/stackless/yield.h>
#include <magic/coroutine/stackless/fire.h>
#include <magic/coroutine/stackless/sync/mutex.h>
#include <magic/coroutine/stackless/sync/waitgroup.h>

#include <magic/executors/manual.h>
#include <magic/executors/thread_pool.h>

#include <twist/test/plate.hpp>

using namespace magic;

//////////////////////////////////////////////////////////////////////

TEST(Stackless_WaitGroup, JustWorks) {
  WaitGroup wg;
  wg.Add(1);
  wg.Done();
}

TEST(Stackless_WaitGroup, OneWaiter) {
  ManualExecutor scheduler;

  WaitGroup wg;

  bool waiter_done = false;
  bool worker_done = false;

  // Waiter

  auto waiter_task = [&]() -> Task<> {
    co_await DispatchTo(scheduler);
    co_await wg.WaitAsync();
    assert(worker_done);
    waiter_done = true;
  };

  FireAndForget(waiter_task());

  // Worker

  auto worker_task = [&]() -> Task<> {
    co_await DispatchTo(scheduler);

    for (size_t index = 0; index < 10; ++index) {
      co_await Yield(scheduler);
    }

    worker_done = true;

    wg.Done();
  };

  FireAndForget(worker_task());

  wg.Add();
  scheduler.RunAll();

  ASSERT_TRUE(waiter_done);
  ASSERT_TRUE(worker_done);
}

//////////////////////////////////////////////////////////////////////

TEST(Stackless_WaitGroup, ManyWaiters) {
  ManualExecutor scheduler;

  WaitGroup wg;

  static const size_t kWorkers = 3;
  static const size_t kWaiters = 4;
  static const size_t kYields = 10;

  size_t waiters_done = 0;
  size_t workers_done = 0;

  auto waiter_task = [&]() -> Task<> {
    co_await DispatchTo(scheduler);
    co_await wg.WaitAsync();
    assert(workers_done == kWorkers);
    ++waiters_done;
  };

  for (size_t i = 0; i < kWaiters; ++i) {
    FireAndForget(waiter_task());
  }

  auto worker_task = [&]() -> Task<> {
    co_await DispatchTo(scheduler);
    for (size_t i = 0; i < kYields; ++i) {
      co_await Yield(scheduler);
    }
    ++workers_done;
    wg.Done();
  };

  wg.Add(kWorkers);
  for (size_t j = 0; j < kWorkers; ++j) {
    FireAndForget(worker_task());
  }

  size_t steps = scheduler.RunAll();

  ASSERT_EQ(workers_done, kWorkers);
  ASSERT_EQ(waiters_done, kWaiters);

  ASSERT_GE(steps, kWaiters + kWorkers * kYields);
}

//////////////////////////////////////////////////////////////////////

TEST(Stackless_WaitGroup, BlockingWait) {
  ThreadPool scheduler{/*threads=*/4};

  WaitGroup wg;

  std::atomic<size_t> workers = 0;
  static const size_t kWorkers = 3;

  wg.Add(kWorkers);

  auto get_waiter_task = [&]() -> Task<> {
    co_await DispatchTo(scheduler);
    co_await wg.WaitAsync();
    assert(workers.load() == kWorkers);
    co_return;
  };

  auto get_worker_task = [&]() -> Task<> {
    co_await DispatchTo(scheduler);
    std::this_thread::sleep_for(1s);
    ++workers;
    wg.Done();
  };

  ProcessCPUTimer timer;

  FireAndForget(get_waiter_task());
  for (size_t i = 0; i < kWorkers; ++i) {
    FireAndForget(get_worker_task());
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
    WaitGroup wg;

    // Number of completed waiters
    std::atomic<size_t> waiters_done{0};
    std::atomic<size_t> workers_done{0};

    wg.Add(workers);

    // Waiters

    auto get_waiter_task = [&]() -> Task<> {
      co_await DispatchTo(scheduler);
      co_await wg.WaitAsync();
      waiters_done.fetch_add(1);
    };

    for (size_t i = 0; i < waiters; ++i) {
      FireAndForget(get_waiter_task());
    }

    // Workers

    auto get_worker_task = [&]() -> Task<> {
      co_await DispatchTo(scheduler);
      workers_done.fetch_add(1);
      wg.Done();
    };

    for (size_t j = 0; j < workers; ++j) {
      FireAndForget(get_worker_task());
    }

    scheduler.WaitIdle();

    ASSERT_EQ(waiters_done.load(), waiters);
    ASSERT_EQ(workers_done.load(), workers);
  }

  scheduler.Stop();
}

TEST(Stackless_WaitGroup, StressTest_1_1) {
  StressTest1(1, 1, 5s);
}

TEST(Stackless_WaitGroup, StressTest_1_2) {
  StressTest1(2, 2, 5s);
}

TEST(Stackless_WaitGroup, StressTest_1_3) {
  StressTest1(3, 3, 5s);
}

//////////////////////////////////////////////////////////////////////

class Goer {
 public:
  explicit Goer(IExecutor& scheduler, WaitGroup& wg)
      : scheduler_(scheduler), wg_(wg) {
  }

  void Start(size_t steps) {
    steps_left_ = steps;
    Step();
  }

  size_t Steps() const {
    return steps_made_;
  }

 private:
  Task<> NextStep() {
    co_await DispatchTo(scheduler_);

    Step();
    wg_.Done();

    co_return;
  }

  void Step() {
    if (steps_left_ == 0) {
      return;
    }

    ++steps_made_;
    --steps_left_;

    wg_.Add(1);
    FireAndForget(NextStep());
  }

 private:
  IExecutor& scheduler_;
  WaitGroup& wg_;
  size_t steps_left_;
  size_t steps_made_ = 0;
};

void StressTest2(Duration duration) {
  InitializeStressTest();

  ThreadPool scheduler{/*threads=*/4};

  size_t iter = 0;

  while (KeepRunning(duration)) {
    ++iter;

    bool done = false;

    auto tester = [&scheduler, &done, iter]() -> Task<> {
      const size_t steps = 1 + iter % 3;

      // Размещаем wg на куче, но только для того, чтобы
      // AddressSanitizer мог обнаружить ошибку
      // Можно считать, что wg находится на стеке
      auto wg = std::make_unique<WaitGroup>();
      //fibers::WaitGroup wg;

      Goer goer{scheduler, *wg};
      goer.Start(steps);

      co_await wg->WaitAsync();


      assert(goer.Steps() == steps);
      assert(steps > 0);

      done = true;
    };

    FireAndForget(tester());

    scheduler.WaitIdle();

    ASSERT_TRUE(done);
  }

  scheduler.Stop();
}

TEST(Stackless_WaitGroup, StressTest_3) {
  StressTest2( 5s);
}