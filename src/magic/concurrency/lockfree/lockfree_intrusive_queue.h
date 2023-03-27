#pragma once

#include <magic/concurrency/lockfree/lockfree_intrusive_stack.h>

namespace magic {

// MPSCUnboundedLockFreeQueue
// Multi-producer/single-consumer (MPSC) unbounded lock-free queue

template <typename T>
class MPSCLockFreeIntrusiveQueue final {
 public:
  using List = wheels::IntrusiveForwardList<T>;

  // ~ Public Interface

  void Put(T* value) {
    stack_.Push(value);
  }

  List TakeAll() {
    List reversed;
    stack_.ConsumeAll([&reversed](T* value) {
      reversed.PushFront(value);
    });
    return reversed;
  }

  bool HasItems() const {
    return !IsEmpty();
  }

  bool IsEmpty() const {
    return stack_.IsEmpty();
  }

 private:
  MPSCLockFreeIntrusiveStack<T> stack_;
};

}  // namespace magic