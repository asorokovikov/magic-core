#pragma once

#include <magic/concurrency/lockfree/stamped_ptr.h>

#include <algorithm>
#include <atomic>
#include <deque>
#include <memory>
#include <optional>
#include <vector>

namespace magic {

//////////////////////////////////////////////////////////////////////

template <typename T>
class LockFreeStack final {
  struct Node {
    T value;
    StampedPtr<Node> next{};
    std::atomic<int32_t> global = 0;

    void IncrementGlobal(int32_t val) {
      auto old_val = global.fetch_add(val);
      if (old_val + val == 0) {
        delete this;
      }
    }
  };

 public:

  // ~ Public Interface

  void Push(T value) {
    auto node = StampedPtr<Node>{new Node{std::move(value)}, 0};
    while (!head_.CompareExchangeWeak(node->next, node)) {
    }
  }

  std::optional<T> TryPop() {
    std::optional<T> result = std::nullopt;

    while (true) {
      auto current = AcquireRef();
      if (!current) {
        break;
      }

      auto head = current;
      if (head_.CompareExchangeWeak(head, head->next)) {
        result = std::move(current->value);
        current->IncrementGlobal(current.stamp - 1);
        break;
      }

      current->IncrementGlobal(-1);
    }

    return result;
  }

  template <typename Func>
  void ConsumeAll(Func func) {
    auto current = head_.Load();
    while (!head_.CompareExchangeWeak(current, {nullptr})) {
    }

    while (current) {
      auto next = current->next;
      func(std::move(current->value));
      current->IncrementGlobal(current.stamp);
      current = next;
    }
  }

  bool IsEmpty() const {
    return head_.Load().IsNull();
  }

  ~LockFreeStack() {
    DestroySelf();
  }

  //////////////////////////////////////////////////////////////////////

 private:
  void DestroySelf() {
    auto current = head_.Load();
    while (current) {
      auto to_delete = current.raw_ptr;
      current = current->next;
      delete to_delete;
    }
  }

  StampedPtr<Node> AcquireRef() {
    auto current = head_.Load();
    while (!head_.CompareExchangeWeak(current, current.IncrementStamp())) {
    }
    return current.IncrementStamp();
  }

 private:
  AtomicStampedPtr<Node> head_;
};

//////////////////////////////////////////////////////////////////////


}  // namespace magic