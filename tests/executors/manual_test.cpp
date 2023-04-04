#include <gtest/gtest.h>

#include <magic/executors/manual.h>
#include <magic/executors/execute.h>
#include <magic/common/random.h>

#include <fmt/core.h>

using namespace magic;

//////////////////////////////////////////////////////////////////////

TEST(ManualExecutor, JustWorks) {
  ManualExecutor manual;
  auto value = Random::Next(100);
  fmt::println("Generated new value: {}", value);

  size_t step = 0;

  ASSERT_FALSE(manual.HasTasks());

  ASSERT_FALSE(manual.RunNext());
  ASSERT_EQ(manual.RunAtMost(99), 0);

  Execute(manual, [&]() {
    step = 1;
  });

  ASSERT_TRUE(manual.HasTasks());
  ASSERT_EQ(manual.PendingTasks(), 1);

  ASSERT_EQ(step, 0);

  Execute(manual, [&]() {
    step = 2;
  });

  ASSERT_EQ(manual.PendingTasks(), 2);

  ASSERT_EQ(step, 0);

  ASSERT_TRUE(manual.RunNext());

  ASSERT_EQ(step, 1);

  ASSERT_TRUE(manual.HasTasks());
  ASSERT_EQ(manual.PendingTasks(), 1);

  Execute(manual, [&]() {
    step = 3;
  });

  ASSERT_EQ(manual.PendingTasks(), 2);

  ASSERT_EQ(manual.RunAtMost(99), 2);
  ASSERT_EQ(step, 3);

  ASSERT_FALSE(manual.HasTasks());
  ASSERT_FALSE(manual.RunNext());
}

//////////////////////////////////////////////////////////////////////

class Looper {
 public:
  explicit Looper(IExecutor& e, size_t iters)
      : executor_(e), iters_left_(iters) {
  }

  void Iter() {
    --iters_left_;
    if (iters_left_ > 0) {
      Execute(executor_, [this]() {
        Iter();
      });
    }
  }

 private:
  IExecutor& executor_;
  size_t iters_left_;
};

//////////////////////////////////////////////////////////////////////

TEST(ManualExecutor, RunAtMost) {
  ManualExecutor manual;
  Looper looper{manual, 256};

  Execute(manual, [&looper]() {
    looper.Iter();
  });

  size_t tasks = 0;
  do {
    tasks += manual.RunAtMost(7);
  } while (manual.HasTasks());

  ASSERT_EQ(tasks, 256);
}

TEST(ManualExecutor, RunAll) {
  ManualExecutor manual;
  Looper looper{manual, 117};

  Execute(manual, [&looper]() {
    looper.Iter();
  });

  ASSERT_EQ(manual.RunAll(), 117);
}