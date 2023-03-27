#include <gtest/gtest.h>

#include <magic/concurrency/lockfree/stack.h>
#include <magic/common/random.h>
#include <magic/common/stopwatch.h>

#include <twist/test/race.hpp>
#include <twist/test/lockfree.hpp>

#include <wheels/intrusive/forward_list.hpp>

#include <fmt/core.h>
#include <limits>

using namespace magic;
using namespace twist::test;

template <typename T>
using Stack = LockFreeStack<T>;

//////////////////////////////////////////////////////////////////////

bool KeepRunning(Duration duration = 10s) {
  static thread_local Stopwatch sw;
  return sw.Elapsed() < duration;
}

//////////////////////////////////////////////////////////////////////

TEST(LockFreeStack, JustWorks) {
  LockFreeStack<std::string> stack;
  stack.Push("Hello");

  auto value = stack.TryPop();
  ASSERT_TRUE(value);
  ASSERT_EQ(*value, "Hello");

  auto empty = stack.TryPop();
  ASSERT_FALSE(empty);
}

TEST(LockFreeStack, Lifo) {
  LockFreeStack<int> stack;

  stack.Push(1);
  stack.Push(2);
  stack.Push(3);

  ASSERT_EQ(*stack.TryPop(), 3);
  ASSERT_EQ(*stack.TryPop(), 2);
  ASSERT_EQ(*stack.TryPop(), 1);

  ASSERT_FALSE(stack.TryPop());
}

////////////////////////////////////////////////////////////////////////

TEST(LockFreeStack, Dtor) {
  LockFreeStack<std::string> stack;

  stack.Push("Hello");
  stack.Push("World");
}

////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////

TEST(LockFreeStack, MoveOnly) {
  LockFreeStack<MoveOnly> stack;

  stack.Push(MoveOnly(1));
  ASSERT_TRUE(stack.TryPop());
}

////////////////////////////////////////////////////////////////////////

TEST(LockFreeStack, TwoStacks) {
  LockFreeStack<int> stack_1;
  LockFreeStack<int> stack_2;

  stack_1.Push(3);
  stack_2.Push(11);
  ASSERT_EQ(*stack_1.TryPop(), 3);
  ASSERT_EQ(*stack_2.TryPop(), 11);
}

////////////////////////////////////////////////////////////////////////

TEST(LockFreeStack, ConsumeAll) {
  LockFreeStack<int> stack;

  stack.Push(1);
  stack.Push(2);
  stack.Push(3);

  auto index = 3;
  stack.ConsumeAll([&](int value) {
    ASSERT_EQ(value, index--);
  });

  ASSERT_FALSE(stack.TryPop());
}

//////////////////////////////////////////////////////////////////////

template <typename T>
struct Counted {
  static std::atomic<size_t> object_count;
  static size_t obj_count_limit;

  Counted() {
    IncrementCount();
  }

  Counted(const Counted& /*that*/) {
    IncrementCount();
  }

  Counted(Counted&& /*that*/) {
    IncrementCount();
  }

  Counted& operator=(const Counted& that) = default;
  Counted& operator=(Counted&& that) = default;

  ~Counted() {
    DecrementCount();
  }

  static void SetLiveLimit(size_t count) {
    obj_count_limit = count;
  }

  static size_t LiveObjectCount() {
    return object_count.load();
  }

 private:
  static void IncrementCount() {
    ASSERT_TRUE(object_count.fetch_add(1) + 1 < obj_count_limit)
        << "Too many alive test objects: " << object_count;
  }

  static void DecrementCount() {
    object_count.fetch_sub(1);
  }
};

template <typename T>
std::atomic<size_t> Counted<T>::object_count = 0;

template <typename T>
size_t Counted<T>::obj_count_limit = std::numeric_limits<size_t>::max();

////////////////////////////////////////////////////////////////////////

struct TestObject : public Counted<TestObject> {
  size_t value;

  explicit TestObject(size_t _value) : value(_value) {
  }

  static TestObject Make() {
    static const size_t kValueRange = 1000007;
    return TestObject{Random::Next(kValueRange)};
  }
};

////////////////////////////////////////////////////////////////////////

void StressTest1(Duration duration) {
  ReportProgressFor<LockFreeStack<int>> stack;

  std::atomic<size_t> pushed{0};
  std::atomic<size_t> popped{0};

  static const int kThreads = 5;

  Race race;
  for (size_t index = 0; index < kThreads; ++index) {
    race.Add([&, index] {
      twist::rt::fault::GetAdversary()->EnablePark();

      int value = index;
      while(KeepRunning(duration)) {
        pushed += value;
        stack->Push(value);

        value = stack->TryPop().value();
        popped += value;
      }
    });
  }

  race.Run();

  fmt::println("Pushed: {}", pushed.load());
  fmt::println("Popped: {}", popped.load());
}

////////////////////////////////////////////////////////////////////////

void StressTest(size_t threads, size_t batch_size_limit, Duration duration) {
  twist::test::ReportProgressFor<LockFreeStack<TestObject>> stack;

  std::atomic<size_t> ops{0};
  std::atomic<size_t> pushed{0};
  std::atomic<size_t> popped{0};

  TestObject::SetLiveLimit(1024);

  twist::test::Race race;

  for (size_t i = 0; i < threads; ++i) {
    race.Add([&]() {
      twist::test::EnablePark guard;

      while (KeepRunning(duration)) {
        size_t batch_size = Random::Next(1, batch_size_limit);

        // Push
        for (size_t j = 0; j < batch_size; ++j) {
          auto obj = TestObject::Make();
          stack->Push(obj);
          pushed.fetch_add(obj.value);
          ++ops;
        }

        // Pop
        for (size_t j = 0; j < batch_size; ++j) {
          auto obj = stack->TryPop();
          ASSERT_TRUE(obj);
          popped.fetch_add(obj->value);
        }
      }
    });
  }

  race.Run();

  std::cout << "Operations #: " << ops.load() << std::endl;
  std::cout << "Pushed: " << pushed.load() << std::endl;
  std::cout << "Popped: " << popped.load() << std::endl;

  ASSERT_EQ(pushed.load(), popped.load());

  // Stack is empty
  ASSERT_FALSE(stack->TryPop());

  std::cout << "Live objects = " << TestObject::LiveObjectCount() << std::endl;
  // Memory leak
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

////////////////////////////////////////////////////////////////////////

void StressTestConsumeAll(size_t threads, size_t batch_size_limit, Duration duration) {
  twist::test::ReportProgressFor<LockFreeStack<TestObject>> stack;

  std::atomic<size_t> ops{0};
  std::atomic<size_t> pushed{0};
  std::atomic<size_t> popped{0};

  TestObject::SetLiveLimit(1024);

  twist::test::Race race;

  for (size_t i = 0; i < threads; ++i) {
    race.Add([&]() {
      twist::test::EnablePark guard;

      while (KeepRunning(duration)) {
        size_t batch_size = Random::Next(1, batch_size_limit);

        // Push
        for (size_t j = 0; j < batch_size; ++j) {
          auto obj = TestObject::Make();
          stack->Push(obj);
          pushed.fetch_add(obj.value);
          ++ops;
        }

        // Pop
        stack->ConsumeAll([&] (auto&& object) {
          popped.fetch_add(object.value);
        });
      }
    });
  }

  race.Run();

  std::cout << "Operations #: " << ops.load() << std::endl;
  std::cout << "Pushed: " << pushed.load() << std::endl;
  std::cout << "Popped: " << popped.load() << std::endl;

  ASSERT_EQ(pushed.load(), popped.load());

  // Stack is empty
  ASSERT_FALSE(stack->TryPop());

  std::cout << "Live objects = " << TestObject::LiveObjectCount() << std::endl;
  // Memory leak
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

//////////////////////////////////////////////////////////////////////


TEST(LockFreeStack, Stress0) {
  StressTest1(5s);
}

TEST(LockFreeStack, Stress1) {
  StressTest(2, 2, 5s);
}

TEST(LockFreeStack, Stress2) {
  StressTest(5, 1, 5s);
}

TEST(LockFreeStack, Stress3) {
  StressTest(5, 3, 5s);
}

TEST(LockFreeStack, Stress4) {
  StressTest(5, 5, 5s);
}

TEST(LockFreeStack, Stress5) {
  StressTestConsumeAll(2, 2, 5s);
}

TEST(LockFreeStack, Stress6) {
  StressTestConsumeAll(5, 1, 5s);
}

TEST(LockFreeStack, Stress7) {
  StressTestConsumeAll(5, 3, 5s);
}

TEST(LockFreeStack, Stress8) {
  StressTestConsumeAll(5, 5, 5s);
}