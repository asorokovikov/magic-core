#include <gtest/gtest.h>
#include "../test_helper.h"

#include <magic/common/cpu_time.h>
#include <magic/common/unit.h>

#include <magic/fibers/sync/mutex.h>

#include <magic/executors/manual.h>
#include <magic/executors/thread_pool.h>

#include <magic/futures/core/future.h>
#include <magic/futures/execute.h>
#include <magic/futures/get.h>

//////////////////////////////////////////////////////////////////////

auto TimeoutError() {
  return std::make_error_code(std::errc::timed_out);
}

//////////////////////////////////////////////////////////////////////

TEST(Futures, JustWorks) {
  auto [f, p] = MakeContract<int>();
  ASSERT_FALSE(f.IsReady());

  std::move(p).SetValue(42);
  ASSERT_TRUE(f.IsReady());

  auto result = std::move(f).GetResult();
  ASSERT_TRUE(result.IsOk());
  ASSERT_EQ(*result, 42);
}

TEST(Futures, Invalid) {
  auto f = Future<int>::Invalid();
  ASSERT_FALSE(f.IsValid());
}

TEST(Futures, MoveOnly) {
  auto [f, p] = MakeContract<MoveOnly>();
  ASSERT_FALSE(f.IsReady());

  std::move(p).SetValue(MoveOnly{"Test"});
  ASSERT_TRUE(f.IsReady());

  auto result = std::move(f).GetResult();
  ASSERT_EQ(result->data, "Test");
}

TEST(Futures, SetError) {
  auto [f, p] = MakeContract<int>();
  ASSERT_FALSE(f.IsReady());

  std::move(p).SetError(TimeoutError());
  ASSERT_TRUE(f.IsReady());

  auto result = std::move(f).GetResult();
  ASSERT_TRUE(result.HasError());
  ASSERT_EQ(result.ErrorCode(), TimeoutError());
}

 TEST(Futures, SetException) {
   auto [f, p] = MakeContract<int>();

   try {
     throw std::bad_exception();
   } catch (...) {
     std::move(p).SetError(std::current_exception());
   }

   auto result = std::move(f).GetResult();

   ASSERT_TRUE(result.HasError());
   ASSERT_THROW(result.ValueOrThrow(), std::bad_exception);
 }

TEST(Futures, ExecuteValue) {
  ManualExecutor manual;

  auto f = futures::Execute(manual, []() {
    return 7;
  });

  manual.RunNext();

  ASSERT_TRUE(f.IsReady());
  ASSERT_EQ(std::move(f).GetResult().ValueOrThrow(), 7);
}

// TEST(Futures, WaitValue) {
//   auto f = futures::Execute(GetInlineExecutor(), []() {
//     return 7;
//   });
//
//   ASSERT_TRUE(f.IsReady());
//
//   auto result = futures::WaitValue(std::move(f));
//   ASSERT_EQ(result, 7);
// }

TEST(Futures, ExecuteError) {
  ManualExecutor manual;

  auto f = futures::Execute(manual, []() -> Unit {
    throw std::bad_exception();
  });

  manual.RunNext();

  ASSERT_TRUE(f.IsReady());
  auto result = std::move(f).GetResult();

  ASSERT_TRUE(result.HasError());
  ASSERT_THROW(result.ThrowIfError(), std::bad_exception);
}

TEST(Futures, Subscribe1) {
  auto [f, p] = MakeContract<int>();
  bool done = false;

  std::move(f).Subscribe([&done](auto result) {
    ASSERT_EQ(*result, 7);
    done = true;
  });

  ASSERT_FALSE(done);

  // Run callback here
  std::move(p).SetValue(7);

  ASSERT_TRUE(done);
}

TEST(Futures, Subscribe2) {
  auto [f, p] = MakeContract<int>();

  std::move(p).SetValue(12);

  bool done = false;

  // Run callback immediately
  std::move(f).Subscribe([&done](auto result) {
    ASSERT_EQ(*result, 12);
    done = true;
  });

  ASSERT_TRUE(done);
}

TEST(Futures, Subscribe3) {
  ThreadPool pool{4};

  auto [f, p] = MakeContract<int>();

  Execute(pool, [p = std::move(p)]() mutable {
    std::this_thread::sleep_for(1s);
    std::move(p).SetValue(7);
  });

  bool done = false;

  std::move(f).Subscribe([&](auto result) {
    ASSERT_EQ(*result, 7);
    done = true;
  });

  pool.WaitIdle();

  ASSERT_TRUE(done);

  pool.Stop();
}

TEST(Futures, CallbackRunOnce) {
  auto [f, p] = MakeContract<int>();
  size_t done = 0;

  std::move(f).Subscribe([&done](auto result) {
    ASSERT_EQ(*result, 7);
    ++done;
  });

  ASSERT_EQ(done, 0);

  // Run callback here
  std::move(p).SetValue(7);

  ASSERT_EQ(done, 1);
}

TEST(Futures, SubscribeVia) {
  ManualExecutor manual;

  auto [f, p] = MakeContract<int>();

  std::move(p).SetValue(11);

  bool done = false;

  std::move(f).Via(manual).Subscribe([&](auto result) {
    ASSERT_EQ(*result, 11);
    done = true;
  });

  ASSERT_FALSE(done);
  ASSERT_EQ(manual.RunAll(), 1);
  ASSERT_TRUE(done);
}

TEST(Futures, BlockingGet1) {
  auto [f, p] = MakeContract<int>();

  std::move(p).SetValue(3);

  auto value = futures::WaitValue(std::move(f));
  ASSERT_EQ(value, 3);
}

TEST(Futures, BlockingGet2) {
  auto [f, p] = MakeContract<int>();

  ThreadPool pool{4};

  Execute(pool, [p = std::move(p)]() mutable {
    std::this_thread::sleep_for(2s);
    std::move(p).SetValue(17);
  });

  {
    CpuTimeBudgetGuard cpu_time_budget{100ms};
    ASSERT_EQ(futures::WaitValue(std::move(f)), 17);
    fmt::println("cpu time usage: {}", cpu_time_budget.Usage());
  }

  pool.Stop();
}

TEST(Futures, Then1) {
  auto [f, p] = MakeContract<int>();

  auto g = std::move(f).Then([](int value) {
    return value + 1;
  });

  std::move(p).SetValue(3);
  ASSERT_EQ(std::move(g).GetResult().ValueOrThrow(), 4);
}

TEST(Futures, ThenRunOnce) {
  auto [f, p] = MakeContract<int>();
  size_t done = 0;

  auto g = std::move(f).Then([&](int value) {
    ++done;
    return value + 1;
  });

  ASSERT_EQ(done, 0);

  std::move(p).SetValue(3);
  ASSERT_EQ(std::move(g).GetResult().ValueOrThrow(), 4);

  ASSERT_EQ(done, 1);
}

TEST(Futures, Then2) {
  auto pool = ThreadPool{4};
  auto budget = CpuTimeBudgetGuard(100ms);

  auto f1 = futures::Execute(pool, []() {
    std::this_thread::sleep_for(1s);
    return 42;
  });

  auto f2 = std::move(f1).Then([](int value) {
    return value + 1;
  });

  ASSERT_EQ(futures::WaitValue(std::move(f2)), 43);

  pool.Stop();
}

TEST(Futures, Then3) {
  auto pool = ThreadPool{4};
  auto budget = CpuTimeBudgetGuard(100ms);

  auto [f, p] = MakeContract<int>();

  std::move(p).SetValue(11);

  auto f2 = std::move(f).Via(pool).Then([&](int value) {
    assert(ThreadPool::Current() == &pool);
    return value + 1;
  });

  int value = futures::WaitValue(std::move(f2));

  ASSERT_EQ(value, 12);

  pool.Stop();
}

TEST(Futures, Then4) {
  auto [f, p] = MakeContract<int>();
  auto g = std::move(f).Then([](auto) -> Unit {
    return {};
  });

  std::move(p).SetError(TimeoutError());
  ASSERT_TRUE(std::move(g).GetResult().HasError());
}

// Then chaining

TEST(Futures, Pipeline) {
  ManualExecutor manual;

  bool done = false;

  futures::Execute(manual,
                   []() {
                     return 1;
                   })
      .Then([](int value) {
        return value + 1;
      })
      .Then([](int value) {
        return value + 2;
      })
      .Then([](int value) {
        return value + 3;
      })
      .Subscribe([&](auto result) {
        fmt::println("value={}", *result);
        done = true;
      });

  size_t tasks = manual.RunAll();
  ASSERT_EQ(tasks, 5);

  ASSERT_TRUE(done);
}

// Then chaining

TEST(Futures, PipelineError) {
  ManualExecutor manual;

  bool done = false;

  futures::Execute(manual,
                   []() {
                     return 1;
                   })
      .Then([](int) -> int {
        throw std::bad_exception();
      })
      .Then([](int) -> int {
        std::abort();  // Skipped
      })
      .Then([](int) -> int {
        std::abort();  // Skipped
      })
      .Subscribe([&done](Result<int> result) {
        ASSERT_TRUE(result.HasError());
        done = true;
      });

  size_t tasks = manual.RunAll();
  ASSERT_EQ(tasks, 5);

  ASSERT_TRUE(done);
}

// Then + Recover

TEST(Futures, PipelineRecover) {
  auto pool = ManualExecutor();
  auto budget = CpuTimeBudgetGuard(100ms);
  auto done = false;

  futures::Execute(pool,
                   []() {
                     return 1;
                   })
      .Then([](int) -> int {
        throw std::bad_exception();
      })
      .Then([](int) -> int {
        std::abort();  // Skipped
      })
      .Recover([](Error error) {
        assert(error.HasError());
        return make_result::Ok(7);
      })
      .Then([](int value) {
        return value + 1;
      })
      .Subscribe([&done](Result<int> result) {
        ASSERT_EQ(*result, 8);
        done = true;
      });

  size_t tasks = pool.RunAll();

  ASSERT_EQ(tasks, 6);
  ASSERT_TRUE(done);
}

TEST(Futures, PipelineRecoverWithoutError) {
  auto manual = ManualExecutor();
  auto budget = CpuTimeBudgetGuard(100ms);

  bool done = false;

  futures::Execute(manual,
                   []() {
                     return 1;
                   })
      .Then([](int result) -> int {
        return result * 2;
      })
      .Then([](int result) -> int {
        return result * 10;
      })
      .Recover([](Error) {
        return make_result::Ok(7);
      })
      .Then([](int value) {
        return value + 1;
      })
      .Subscribe([&done](Result<int> result) {
        ASSERT_EQ(*result, 21);
        done = true;
      });

  size_t tasks = manual.RunAll();

  ASSERT_EQ(tasks, 6);
  ASSERT_TRUE(done);
}

// Via + Then

TEST(Futures, Via) {
  auto manual1 = ManualExecutor();
  auto manual2 = ManualExecutor();

  auto [f, p] = MakeContract<int>();

  int step = 0;

  auto f2 = std::move(f)
                .Via(manual1)
                .Then([&](int value) {
                  step = 1;
                  return value + 1;
                })
                .Then([&](int value) {
                  step = 2;
                  return value + 2;
                })
                .Via(manual2)
                .Then([&](int value) {
                  step = 3;
                  return value + 3;
                });

  // Launch pipeline
  std::move(p).SetValue(0);

  ASSERT_EQ(manual1.RunAll(), 2);
  ASSERT_EQ(step, 2);
  ASSERT_EQ(manual2.RunAll(), 1);
  ASSERT_EQ(step, 3);

  auto f3 = std::move(f2)
                .Then([&](int value) {
                  step = 4;
                  return value + 4;
                })
                .Via(manual1)
                .Then([&](int value) {
                  step = 5;
                  return value + 5;
                });

  ASSERT_EQ(manual2.RunAll(), 1);
  ASSERT_EQ(step, 4);

  ASSERT_EQ(manual1.RunAll(), 1);
  ASSERT_EQ(step, 5);

  ASSERT_TRUE(f3.IsReady());
  auto value = std::move(f3).GetResult().ValueOrThrow();
  ASSERT_EQ(value, 1 + 2 + 3 + 4 + 5);
}

// Asynchronous Then

TEST(Futures, AsyncThen) {
  auto pool1 = ThreadPool{4};
  auto pool2 = ThreadPool{4};
  auto budget = CpuTimeBudgetGuard(100ms);

  auto pipeline = futures::Execute(pool1,
                                   []() -> int {
                                     return 1;
                                   })
                      .Then([&pool2](int value) {
                        return futures::Execute(pool2, [value]() {
                          return value + 1;
                        });
                      })
                      .Then([&pool1](int value) {
                        return futures::Execute(pool1, [value]() {
                          return value + 2;
                        });
                      });

  int value = futures::WaitValue(std::move(pipeline));

  pool1.Stop();
  pool2.Stop();

  ASSERT_EQ(value, 4);
}