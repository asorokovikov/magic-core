#pragma once

#include <magic/concurrency/lockfree/stack.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

// Michael-Scott lock-free queue

// Queue: dummy -> node1 -> node2 -> node3
// head_ -> dummy
// tail -> node3

template <typename T>
class LockFreeQueue final {
 public:
  using List = std::deque<T>;

  LockFreeQueue() {
  }

  // ~ Public Interface

  void Put(T value) {
    main_.Push(std::move(value));
  }

  std::optional<T> Take() {
    if (reserve_.IsEmpty()) {
      main_.ConsumeAll([this](T value) {
        reserve_.Push(std::move(value));
      });
    }

    return reserve_.TryPop();
  }

  List TakeAll() {
    auto result = std::deque<T>();
    while (auto item = Take()) {
      result.push_back(std::move(item.value()));
    }

    return result;
  }

  bool IsEmpty() const {
    return main_.IsEmpty() && reserve_.IsEmpty();
  }

 private:
  LockFreeStack<T> main_;
  LockFreeStack<T> reserve_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic