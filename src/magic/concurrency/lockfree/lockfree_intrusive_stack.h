#pragma once

#include <wheels/intrusive/forward_list.hpp>

#include <atomic>

namespace magic {

//////////////////////////////////////////////////////////////////////

// MPSCUnboundedLockFreeStack

template <typename T>
class MPSCLockFreeIntrusiveStack {
  using Node = wheels::IntrusiveForwardListNode<T>;

 public:

  // ~ Public Interface

  void Push(T* item) {
    Node* node = item;
    while (!head_.compare_exchange_weak(node->next_, node, std::memory_order::release,
                                        std::memory_order::relaxed)) {
    }
  }

  template <typename F>
  void ConsumeAll(F func) {
    auto top = head_.exchange(nullptr, std::memory_order::acquire);
    Node* current = top;
    while (current) {
      Node* next = current->next_;
      func(current->AsItem());
      current = next;
    }
  }

  bool IsEmpty() const {
    return head_.load(std::memory_order::acquire) == nullptr;
  }

  bool HasItems() const {
    return !IsEmpty();
  }

 private:
  std::atomic<Node*> head_ = nullptr;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic