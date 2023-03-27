#pragma once

#include <wheels/intrusive/forward_list.hpp>

#include <atomic>
#include <mutex>
#include <coroutine>
#include <cassert>

namespace magic {

//////////////////////////////////////////////////////////////////////

class Mutex final {
  using State = uint64_t;
  using UniqueLock = std::unique_lock<Mutex>;
  using CoroutinHandle = std::coroutine_handle<>;

  struct States {
    enum _ { Unlocked = 0, LockedNoWaiters = 1, /*LockedWithWaiters = pointer_to_head*/ };
  };

  class [[nodiscard]] Locker final : public wheels::IntrusiveForwardListNode<Locker> {
   public:

    explicit Locker(Mutex& mutex) : mutex_(mutex) {
    }

    // NOLINTNEXTLINE
    bool await_ready() {
      return mutex_.TryLock();
    }

    // NOLINTNEXTLINE
    bool await_suspend(CoroutinHandle handle) {
      handle_ = handle;
      return !mutex_.TryLockOrEnqueue(this);
    }

    // NOLINTNEXTLINE
    UniqueLock await_resume() {
      return UniqueLock(mutex_);
    }

    void Resume() const {
      handle_.resume();
    }

   private:
    CoroutinHandle handle_;
    Mutex& mutex_;
  };

 public:

  // ~ Public Interface

  // Asynchronous
  auto ScopedLock() {
    return Locker{*this};
  }

  bool TryLock() {
    State expected = States::Unlocked;
    return state_.compare_exchange_strong(expected, States::LockedNoWaiters);
  }

  // For TryLock
  void Unlock() {
    Release();
  }

  // For std::unique_lock
  // Do not use directly
  void lock() {  // NOLINT
  }

  void unlock() {  // NOLINT
    Unlock();
  }

  //////////////////////////////////////////////////////////////////////

 private:
  using Node = wheels::IntrusiveForwardListNode<Locker>;

  bool TryLockOrEnqueue(Node* node) {
    while (true) {
      auto state = state_.load();
      if (state == States::Unlocked) {
        if (TryLock()) {
          return true;
        }
        continue;
      } else {
        if (state == States::LockedNoWaiters) {
          node->ResetNext();
        } else {
          node->SetNext((Node*)state);
        }
        if (state_.compare_exchange_strong(state, (State)node)) {
          return false;
        }
      }
    }
  }

  void Release() {
    if (waiters_.HasItems()) {
      ResumeNextWaiter();
      return;
    }
    // head list is empty
    // state = LockNoWaiters | Waiters list
    while (true) {
      State state = state_.load();
      if (state == States::LockedNoWaiters) {
        if (state_.compare_exchange_strong(state, States::Unlocked, std::memory_order::release)) {
          return;
        }
        continue;
      }
      // Wait list
      auto head = (Node*)state_.exchange(States::LockedNoWaiters, std::memory_order::acquire);
      AppendToList(head);
      ResumeNextWaiter();
      return;
    }
  }

  void ResumeNextWaiter() {
    assert(waiters_.HasItems());
    auto next = waiters_.PopFront();
    next->Resume();
  }

  void AppendToList(Node* head) {
    assert(waiters_.IsEmpty());
    auto curr = head;
    while (curr) {
      auto next = curr->Next();
      waiters_.PushBack(curr);
      curr = next;
    }
  }

 private:
  std::atomic<State> state_ = States::Unlocked;
  wheels::IntrusiveForwardList<Locker> waiters_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic