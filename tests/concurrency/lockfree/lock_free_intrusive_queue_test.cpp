#include <gtest/gtest.h>

#include <magic/common/random.h>
#include <magic/common/stopwatch.h>

#include <magic/concurrency/barrier.h>
#include <magic/concurrency/lockfree/lockfree_intrusive_queue.h>

#include <twist/test/race.hpp>
#include <twist/test/lockfree.hpp>

#include <fmt/core.h>
#include <limits>
#include <thread>

using namespace magic;
using namespace twist::test;

template <typename T>
using Queue = MPSCLockFreeIntrusiveQueue<T>;

//////////////////////////////////////////////////////////////////////

bool KeepRunning(Duration duration = 10s) {
  static thread_local Stopwatch sw;
  return sw.Elapsed() < duration;
}

template <typename T>
struct NodeItem : wheels::IntrusiveForwardListNode<NodeItem<T>> {
  T value;
  NodeItem(T initial) : value(std::move(initial)) {
  }
};

//////////////////////////////////////////////////////////////////////

TEST(LockFreeIntrusiveQueue, JustWorks) {
  Queue<NodeItem<int>> queue;

  auto item1 = NodeItem(42);
  queue.Put(&item1);

  ASSERT_FALSE(queue.IsEmpty());
  ASSERT_EQ(queue.TakeAll().PopFront()->value, 42);
  ASSERT_TRUE(queue.IsEmpty());
}

//////////////////////////////////////////////////////////////////////

TEST(LockFreeIntrusiveQueue, FifoSmall) {
  Queue<NodeItem<int>> queue;

  auto item1 = NodeItem{1};
  auto item2 = NodeItem{2};
  auto item3 = NodeItem{3};

  queue.Put(&item1);
  queue.Put(&item2);
  queue.Put(&item3);

  auto items = queue.TakeAll();

  ASSERT_EQ(items.PopFront()->value, 1);
  ASSERT_EQ(items.PopFront()->value, 2);
  ASSERT_EQ(items.PopFront()->value, 3);

  ASSERT_TRUE(queue.IsEmpty());
}

//////////////////////////////////////////////////////////////////////

TEST(LockFreeIntrusiveQueue, FifoLarge) {
  static const size_t kMaxItems = 12345;

  Queue<NodeItem<int>> queue;

  auto barrier = CyclicBarrier(2);

  // Producer

  std::thread producer([&] {
    barrier.ArriveAndWait();
    for (size_t index = 0; index < kMaxItems; ++index) {
      auto node = new NodeItem<int>(index);
      queue.Put(node);
    }
  });

  // Consumer

  std::thread consumer([&] {
    barrier.ArriveAndWait();
    size_t index = 0;
    while (index < kMaxItems) {
      auto items = queue.TakeAll();
      while (items.HasItems()) {
        auto current = items.PopFront();
        ASSERT_EQ(current->value, index++);
        delete current;
      }
    }
  });

  producer.join();
  consumer.join();
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

//////////////////////////////////////////////////////////////////////


void StressTestLiveObject(size_t threads, size_t batch_size_limit, Duration duration) {
  twist::test::ReportProgressFor<Queue<TestObject>> queue;

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
          queue->Put(obj);
          ++ops;
        }

        // Pop
        auto items = queue->TakeAll();
        while (items.HasItems()) {
          auto current = items.PopFront();
          popped.fetch_add(current->value);
          delete current;
        }
      }
    });
  }

  race.Run();

  std::cout << "Operations #: " << ops.load() << std::endl;
  std::cout << "Pushed: " << pushed.load() << std::endl;
  std::cout << "Popped: " << popped.load() << std::endl;

  ASSERT_EQ(pushed.load(), popped.load());
  ASSERT_TRUE(queue->IsEmpty());

  std::cout << "Live objects = " << TestObject::LiveObjectCount() << std::endl;
  // Memory leak
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

//////////////////////////////////////////////////////////////////////

TEST(LockFreeIntrusiveQueue, Stress6) {
  StressTestLiveObject(5, 1, 5s);
}

TEST(LockFreeIntrusiveQueue, Stress7) {
  StressTestLiveObject(5, 3, 5s);
}

TEST(LockFreeIntrusiveQueue, Stress8) {
  StressTestLiveObject(5, 5, 5s);
}