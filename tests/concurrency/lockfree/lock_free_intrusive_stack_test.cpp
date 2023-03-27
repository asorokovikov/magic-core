#include <gtest/gtest.h>

#include <magic/concurrency/lockfree/lockfree_intrusive_stack.h>
#include <magic/common/random.h>
#include <magic/common/stopwatch.h>

#include <twist/test/race.hpp>
#include <twist/test/lockfree.hpp>

#include <fmt/core.h>
#include <limits>

using namespace magic;
using namespace twist::test;

template <typename T>
using Stack = magic::MPSCLockFreeIntrusiveStack<T>;

//////////////////////////////////////////////////////////////////////

bool KeepRunning(Duration duration = 10s) {
  static thread_local Stopwatch sw;
  return sw.Elapsed() < duration;
}

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

//////////////////////////////////////////////////////////////////////

template <typename T>
struct NodeItem : wheels::IntrusiveForwardListNode<NodeItem<T>> {
  T value;
  NodeItem(T initial) : value(std::move(initial)) {
  }
};

//////////////////////////////////////////////////////////////////////

TEST(LockFreeIntrusiveStack, JustWorks) {
  const static std::string kString = "Hello";

  Stack<NodeItem<std::string>> stack;

  auto item = NodeItem(kString);
  stack.Push(&item);
  ASSERT_FALSE(stack.IsEmpty());
  stack.ConsumeAll([](auto item) {
    ASSERT_TRUE(item);
    ASSERT_EQ(item->value, kString);
  });
  ASSERT_TRUE(stack.IsEmpty());
}

TEST(LockFreeIntrusiveStack, Lifo) {
  Stack<NodeItem<int>> stack;

  auto item1 = NodeItem{1};
  auto item2 = NodeItem{2};
  auto item3 = NodeItem{3};

  stack.Push(&item1);
  stack.Push(&item2);
  stack.Push(&item3);

  auto index = 3;
  stack.ConsumeAll([&](auto item) {
    ASSERT_TRUE(item);
    ASSERT_EQ(item->value, index--);
  });

  ASSERT_TRUE(stack.IsEmpty());
}

//////////////////////////////////////////////////////////////////////////

TEST(LockFreeIntrusiveStack, Dtor) {
  Stack<NodeItem<std::string>> stack;

  auto item1 = NodeItem<std::string>("Hello");
  stack.Push(&item1);

  auto item2 = NodeItem<std::string>("World");
  stack.Push(&item2);
}

//////////////////////////////////////////////////////////////////////////

TEST(LockFreeIntrusiveStack, TwoStacks) {
  Stack<NodeItem<int>> stack1;
  Stack<NodeItem<int>> stack2;

  auto item1 = NodeItem{1};
  auto item2 = NodeItem{2};

  stack1.Push(&item1);
  stack2.Push(&item2);

  stack1.ConsumeAll([&](auto item) {
    ASSERT_TRUE(item);
    ASSERT_EQ(item, &item1);
  });
  ASSERT_TRUE(stack1.IsEmpty());

  stack2.ConsumeAll([&](auto item) {
    ASSERT_TRUE(item);
    ASSERT_EQ(item, &item2);
  });
  ASSERT_TRUE(stack2.IsEmpty());
}

////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////

struct TestObject : public Counted<TestObject>,
                    public wheels::IntrusiveForwardListNode<TestObject> {
  size_t value;

  explicit TestObject(size_t _value) : value(_value) {
  }

  static TestObject* Make() {
    static const size_t kValueRange = 1000007;
    return new TestObject{Random::Next(kValueRange)};
  }
};

//////////////////////////////////////////////////////////////////////////

void StressTest(Duration duration) {
  ReportProgressFor<Stack<NodeItem<int>>> stack;

  std::atomic<size_t> pushed{0};
  std::atomic<size_t> popped{0};

  static const int kThreads = 5;

  Race race;
  for (size_t index = 0; index < kThreads; ++index) {
    race.Add([&, index] {
      twist::rt::fault::GetAdversary()->EnablePark();

      int value = index;
      while (KeepRunning(duration)) {
        pushed += value;
        auto node = new NodeItem(value);
        stack->Push(node);

        stack->ConsumeAll([&](NodeItem<int>* node) {
          popped += node->value;
          delete node;
        });
      }
    });
  }

  race.Run();

  fmt::println("Pushed: {}", pushed.load());
  fmt::println("Popped: {}", popped.load());
}

TEST(LockFreeIntrusiveStack, Stress) {
  StressTest(5s);
}

//////////////////////////////////////////////////////////////////////

void StressTestLiveObject(size_t threads, size_t batch_size_limit, Duration duration) {
  twist::test::ReportProgressFor<Stack<TestObject>> stack;

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
          pushed.fetch_add(obj->value);
          stack->Push(obj);
          ++ops;
        }

        // Pop
        stack->ConsumeAll([&](auto object) {
          popped.fetch_add(object->value);
          delete object;
        });
      }
    });
  }

  race.Run();

  std::cout << "Operations #: " << ops.load() << std::endl;
  std::cout << "Pushed: " << pushed.load() << std::endl;
  std::cout << "Popped: " << popped.load() << std::endl;

  ASSERT_EQ(pushed.load(), popped.load());
  ASSERT_TRUE(stack->IsEmpty());

  std::cout << "Live objects = " << TestObject::LiveObjectCount() << std::endl;
  // Memory leak
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

TEST(LockFreeIntrusiveStack, Stress5) {
  StressTestLiveObject(2, 2, 5s);
}

TEST(LockFreeIntrusiveStack, Stress6) {
  StressTestLiveObject(5, 1, 5s);
}

TEST(LockFreeIntrusiveStack, Stress7) {
  StressTestLiveObject(5, 3, 5s);
}

TEST(LockFreeIntrusiveStack, Stress8) {
  StressTestLiveObject(5, 5, 5s);
}