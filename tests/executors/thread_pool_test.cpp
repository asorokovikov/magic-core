#include <gtest/gtest.h>

#include <magic/executors/execute.h>
#include <magic/executors/thread_pool.h>
#include <magic/common/stopwatch.h>
#include <magic/common/cpu_time.h>

#include <twist/test/race.hpp>

#include <fmt/core.h>

using namespace magic;

using ThreadPool = magic::ThreadPool;

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, JustWorks) {
  ThreadPool pool{4};

  Execute(pool, []{
    fmt::println("Hello from thread pool!");
  });

  pool.WaitIdle();
  pool.Stop();
}

////////////////////////////////////////////////////////////////////////

TEST(ThreadPool, Wait) {
  ThreadPool pool{4};

  bool done = false;

  Execute(pool, [&]() {
    std::this_thread::sleep_for(1s);
    done = true;
  });

  pool.WaitIdle();
  pool.Stop();

  ASSERT_TRUE(done);
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, MultiWait) {
  ThreadPool pool{1};

  for (size_t index = 0; index < 3; ++index) {
    bool done = false;

    Execute(pool, [&]() {
      std::this_thread::sleep_for(1s);
      done = true;
    });

    pool.WaitIdle();
    ASSERT_TRUE(done);
  }

  pool.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, Exceptions) {
  ThreadPool pool{1};

  Execute(pool, [&]() {
    throw std::runtime_error("Task failed");
  });

  pool.WaitIdle();
  pool.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, ManyTasks) {
  static const size_t kIterations = 17;

  ThreadPool pool{4};

  std::atomic<size_t> actual_value = 0;

  for (size_t it = 0; it < kIterations; ++it) {
    Execute(pool, [&]() {
      actual_value.fetch_add(1);
    });
  }

  pool.WaitIdle();
  pool.Stop();

  ASSERT_EQ(actual_value.load(), kIterations);
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, Parallel) {
  ThreadPool pool{4};

  std::atomic<size_t> value = 0;

  Execute(pool, [&]() {
    std::this_thread::sleep_for(1s);
    value.fetch_add(1);
  });

  Execute(pool, [&]() {
    value.fetch_add(1);
  });

  std::this_thread::sleep_for(100ms);

  ASSERT_EQ(value.load(), 1);

  pool.WaitIdle();
  pool.Stop();

  ASSERT_EQ(value.load(), 2);
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, TwoPools) {
  ThreadPool pool1{1};
  ThreadPool pool2{1};

  auto value = std::atomic<size_t>(0);

  Stopwatch stopwatch;

  Execute(pool1, [&]() {
    std::this_thread::sleep_for(1s);
    value.fetch_add(1);
  });

  Execute(pool2, [&]() {
    std::this_thread::sleep_for(1s);
    value.fetch_add(1);
  });

  pool2.WaitIdle();
  pool2.Stop();

  pool1.WaitIdle();
  pool1.Stop();

  ASSERT_TRUE(stopwatch.Elapsed() < 1500ms);
  ASSERT_EQ(value.load(), 2);
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, Shutdown) {
  ThreadPool pool{3};

  for (size_t it = 0; it < 3; ++it) {
    Execute(pool, [&]() {
      std::this_thread::sleep_for(1s);
    });
  }

  for (size_t it = 0; it < 10; ++it) {
    Execute(pool, [&]() {
      std::this_thread::sleep_for(100s);
    });
  }

  std::this_thread::sleep_for(250ms);

  pool.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, DoNotBurnCPU) {
  ThreadPool pool{4};

  // Warmup
  for (size_t it = 0; it < 4; ++it) {
    Execute(pool, [&]() {
      std::this_thread::sleep_for(100ms);
    });
  }

  ThreadCPUTimer timer;

  std::this_thread::sleep_for(1s);

  pool.WaitIdle();
  pool.Stop();

  ASSERT_TRUE(timer.Elapsed() < 100ms);
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, TooManyThreads) {
  ThreadPool pool{3};

  std::atomic<size_t> value = 0;

  for (size_t it = 0; it < 4; ++it) {
    Execute(pool, [&]() {
      std::this_thread::sleep_for(750ms);
      ++value;
    });
  }

  Stopwatch stopwatch;

  pool.WaitIdle();
  pool.Stop();

  ASSERT_TRUE(stopwatch.Elapsed() > 1000ms && stopwatch.Elapsed() < 2000ms);
  ASSERT_EQ(value.load(), 4);
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, Current) {
  ThreadPool pool{1};

  ASSERT_EQ(ThreadPool::Current(), nullptr);

  Execute(pool, [&]() {
    ASSERT_EQ(ThreadPool::Current(), &pool);
  });

  pool.WaitIdle();
  pool.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, SubmitAfterWait) {
  ThreadPool pool{4};

  bool done = false;

  Execute(pool, [&]() {
    std::this_thread::sleep_for(500ms);
    Execute(*ThreadPool::Current(), [&]() {
      std::this_thread::sleep_for(500ms);
      done = true;
    });
  });

  pool.WaitIdle();
  pool.Stop();

  ASSERT_TRUE(done);
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, SubmitAfterShutdown) {
  ThreadPool pool{4};

  bool done = false;

  Execute(pool, [&]() {
    std::this_thread::sleep_for(500ms);
    Execute(*ThreadPool::Current(), [&]() {
      std::this_thread::sleep_for(500ms);
      done = true;
    });
  });

  pool.Stop();

  ASSERT_FALSE(done);
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, TaskLifetime) {
  ThreadPool pool{4};

  std::atomic<int> dead{0};

  class Task {
   public:
    Task(std::atomic<int>& done) : counter_(done) {
    }
    Task(const Task&) = delete;
    Task(Task&&) = default;

    ~Task() {
      if (done_) {
        counter_.fetch_add(1);
      }
    }

    void operator()() {
      std::this_thread::sleep_for(100ms);
      done_ = true;
    }

   private:
    bool done_{false};
    std::atomic<int>& counter_;
  };

  for (int i = 0; i < 4; ++i) {
    Execute(pool, Task(dead));
  }

  std::this_thread::sleep_for(500ms);
  ASSERT_EQ(dead.load(), 4);

  pool.WaitIdle();
  pool.Stop();
}

//////////////////////////////////////////////////////////////////////

TEST(ThreadPool, Race) {
  ThreadPool pool{4};

  static const size_t rounds = 100500;

  std::atomic<size_t> shared_value{0};
  std::atomic<size_t> round_counter{0};

  for (size_t it = 0; it < rounds; ++it) {
    Execute(pool, [&]() {
      auto old_value = shared_value.load();
      shared_value.store(old_value + 1);

      ++round_counter;
    });
  }

  pool.WaitIdle();
  pool.Stop();

  ASSERT_EQ(round_counter.load(), rounds);
  ASSERT_LE(shared_value.load(), rounds);
}