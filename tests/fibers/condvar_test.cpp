#include <gtest/gtest.h>
#include "../test_helper.h"

#include <magic/fibers/api.h>
#include <magic/fibers/sync/mutex.h>
#include <magic/fibers/sync/condvar.h>

#include <magic/executors/thread_pool.h>

//////////////////////////////////////////////////////////////////////

class Event {
 public:
  void Wait() {
    std::unique_lock lock(mutex_);
    while (!ready_) {
      waiters_.Wait(lock);
    }
  }

  void Fire() {
    std::lock_guard guard(mutex_);
    ready_ = true;
    waiters_.NotifyAll();
  }

 private:
  fibers::Mutex mutex_;
  fibers::CondVar waiters_;

  bool ready_ = false;
};

//////////////////////////////////////////////////////////////////////

class Semaphore {
 public:
  explicit Semaphore(size_t init) : permits_(init) {
  }

  void Acquire() {
    std::unique_lock lock(mutex_);
    while (permits_ == 0) {
      has_permits_.Wait(lock);
    }
    --permits_;
  }

  void Release() {
    std::lock_guard guard(mutex_);
    ++permits_;
    has_permits_.NotifyOne();
  }

 private:
  fibers::Mutex mutex_;
  fibers::CondVar has_permits_;
  size_t permits_;
};

//////////////////////////////////////////////////////////////////////

void EventStressTest(Duration duration = 5s) {
  InitializeStressTest();

  ThreadPool scheduler{/*threads=*/4};

  while (KeepRunning(duration)) {
    Event event;
    std::atomic<size_t> oks{0};

    Go(scheduler, [&]() {
      event.Wait();
      ++oks;
    });

    Go(scheduler, [&]() {
      event.Fire();
    });

    Go(scheduler, [&]() {
      event.Wait();
      ++oks;
    });

    scheduler.WaitIdle();

    ASSERT_EQ(oks.load(), 2);
  }

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

void SemaphoreStressTest(size_t permits, size_t fibers, Duration duration) {
  InitializeStressTest();

  ThreadPool scheduler{/*threads=*/4};

  while (KeepRunning(duration)) {
    Semaphore sema{/*init=*/permits};

    std::atomic<size_t> access_count{0};
    std::atomic<size_t> load{0};

    for (size_t i = 0; i < fibers; ++i) {
      Go(scheduler, [&]() {
        sema.Acquire();

        ++access_count;

        {
          size_t curr_load = load.fetch_add(1) + 1;
          ASSERT_TRUE(curr_load <= permits);

          self::Yield();

          load.fetch_sub(1);
        }

        sema.Release();
      });
    };

    scheduler.WaitIdle();

    ASSERT_EQ(access_count.load(), fibers);
  }

  scheduler.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(CondVar, EventStress) {
  EventStressTest(5s);
}

TEST(CondVar, SemaphoreStress_1) {
  SemaphoreStressTest(1, 2, 5s);
}

TEST(CondVar, SemaphoreStress_2) {
  SemaphoreStressTest(2, 4, 5s);
}

TEST(CondVar, SemaphoreStress_3) {
  SemaphoreStressTest(2, 10, 5s);
}

TEST(CondVar, SemaphoreStress_4) {
  SemaphoreStressTest(5, 16, 5s);
}