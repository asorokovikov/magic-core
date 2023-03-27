#include <gtest/gtest.h>

#include <magic/common/shared_ptr.h>

#include <fmt/core.h>
#include <chrono>
#include <thread>

using namespace magic;

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

//////////////////////////////////////////////////////////////////////

struct TestObject : public Counted<TestObject> {
  TestObject(int v) : value(v) {
  }
  int value;
};

//////////////////////////////////////////////////////////////////////

TEST(SharedPtr, JustWorks) {
  {
    auto sp = MakeShared<TestObject>(7);

    ASSERT_TRUE(sp);
    // Access data
    ASSERT_EQ((*sp).value, 7);
    ASSERT_EQ(sp->value, 7);

    sp.Reset();
    ASSERT_FALSE(sp);
  }
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

TEST(SharedPtr, Copy) {
  {
    auto sp1 = MakeShared<TestObject>(3);
    auto sp2 = sp1;

    ASSERT_TRUE(sp2);
    ASSERT_EQ(sp2->value, 3);

    SharedPtr<TestObject> sp3(sp2);

    ASSERT_TRUE(sp3);
    ASSERT_EQ(sp3->value, 3);

    sp3 = sp3;
    ASSERT_TRUE(sp3);
    ASSERT_EQ(sp3->value, 3);
  }
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

TEST(SharedPtr, Move) {
  {
    auto sp1 = MakeShared<TestObject>(2);
    auto sp2 = std::move(sp1);

    ASSERT_FALSE(sp1);
    ASSERT_TRUE(sp2);
    ASSERT_EQ(sp2->value, 2);
  }
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

TEST(SharedPtr, Share) {
  {
    auto sp1 = MakeShared<TestObject>(3);
    auto sp2 = sp1;

    ASSERT_TRUE(sp2);
    ASSERT_EQ(sp2->value, 3);

    auto sp3 = std::move(sp1);

    ASSERT_TRUE(sp3);
    ASSERT_FALSE(sp1);

    ASSERT_EQ(sp3->value, 3);

    auto sp4 = sp3;
    sp3.Reset();

    ASSERT_TRUE(sp4);
    ASSERT_EQ(sp4->value, 3);
  }
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

TEST(SharedPtr, Nullptr) {
  {
    SharedPtr<TestObject> sp0;
    ASSERT_FALSE(sp0);
    auto sp1 = sp0;
    SharedPtr<TestObject> sp2(sp0);
    SharedPtr<TestObject> sp3(std::move(sp0));

    ASSERT_FALSE(sp1);
    ASSERT_FALSE(sp2);
    ASSERT_FALSE(sp3);
  }
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

//////////////////////////////////////////////////////////////////////

struct ListNode {
  ListNode() = default;

  explicit ListNode(SharedPtr<ListNode> _next) : next(_next) {
  }

  SharedPtr<ListNode> next;
};

size_t ListLength(SharedPtr<ListNode> head) {
  size_t length = 0;
  while (head) {
    ++length;
    head = head->next;
  }
  return length;
}

//////////////////////////////////////////////////////////////////////

TEST(SharedPtr, List) {
  {
    SharedPtr<ListNode> head;
    {
      auto n1 = MakeShared<ListNode>();
      auto n2 = MakeShared<ListNode>(n1);
      auto n3 = MakeShared<ListNode>(n2);
      head = n3;
    }

    ASSERT_EQ(ListLength(head), 3);
  }
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

TEST(SharedPtr, Atomic_Basics) {
  {
    AtomicSharedPtr<TestObject> asp;

    auto sp0 = asp.Load();
    ASSERT_FALSE(sp0);
    {
      auto sp1 = MakeShared<TestObject>(8);
      asp.Store(sp1);
    }

    auto sp2 = asp.Load();

    ASSERT_TRUE(sp2);
    ASSERT_EQ(sp2->value, 8);
  }
  ASSERT_TRUE(TestObject::LiveObjectCount() == 0);
}

TEST(SharedPtr, Atomic_JustWorks) {
  AtomicSharedPtr<TestObject> asp;

  auto sp0 = asp.Load();
  ASSERT_FALSE(sp0);

  auto sp1 = MakeShared<TestObject>(8);

  asp.Store(sp1);

  auto sp2 = asp.Load();

  ASSERT_TRUE(sp1);
  ASSERT_TRUE(sp2);

  ASSERT_EQ(sp2->value, 8);

  auto sp3 = MakeShared<TestObject>(9);

  return ;
  bool success = asp.CompareExchangeWeak(sp1, sp3);
  ASSERT_TRUE(success);

  auto sp4 = asp.Load();
  ASSERT_EQ(sp4->value, sp3->value);

  auto sp5 = MakeShared<TestObject>(7);
  bool failure = asp.CompareExchangeWeak(sp1, sp5);
  ASSERT_FALSE(failure);

  auto sp6 = asp.Load();
  ASSERT_EQ(sp6->value, sp3->value);
}