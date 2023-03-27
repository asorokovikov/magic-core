#pragma once

#include <magic/executors/strand.h>
#include <magic/executors/thread_pool.h>

#include <magic/fibers/api.h>
#include <magic/fibers/core/awaiter.h>
#include <magic/fibers/core/handle.h>

#include <atomic>
#include <optional>
#include <cassert>

namespace magic::fibers {

//////////////////////////////////////////////////////////////////////

class LockFreeMutex final {
  struct States {
    enum _ {
      Unlocked = 0,
      LockedNoWaiters = 1,
      // + WaitNode address
    };
  };

  using State = uint64_t;

  struct WaitNode {
    FiberHandle handle;
    WaitNode* next = nullptr;
  };

  struct LockAwaiter : public IMaybeSuspendAwaiter {
    LockAwaiter(LockFreeMutex& mutex) : mutex_(mutex) {
    }

    // Interface: IMaybeSuspendAwaiter
    bool AwaitSuspend(FiberHandle handle) override {
      assert(handle.IsValid());
      node_.handle = handle;
      if (mutex_.Acquire(&node_)) {
        return true;
      }
      return false;
    }

   private:
    WaitNode node_;
    LockFreeMutex& mutex_;
  };

  struct UnlockAwaiter : ISuspendAwaiter {
    UnlockAwaiter(FiberHandle next) : next_(next) {
    }

    FiberHandle OnCompleted(FiberHandle handle) override {
      auto next = next_; // This awaiter can be destroyed after next line!
      handle.Schedule();
      return next;
    }

   private:
    FiberHandle next_;
  };

  //////////////////////////////////////////////////////////////////////

 public:
  void Lock() {
    if (TryAcquire()) {
      return;  // Fast path
    }
    auto awaiter = LockAwaiter(*this);
    self::Suspend(awaiter);
  }

  bool TryLock() {
    return TryAcquire();
  }

  void Unlock() {
    Release();
  }

  // BasicLockable

  void lock() {
    Lock();
  }

  void unlock() {
    Unlock();
  }

  //////////////////////////////////////////////////////////////////////

 private:
  bool TryAcquire() {
    State current = States::Unlocked;
    return state_.compare_exchange_strong(current, States::LockedNoWaiters,
                                          std::memory_order::acquire);
  }

  bool Acquire(WaitNode* node) {
    while (true) {
      auto state = state_.load();
      if (state == States::Unlocked) {
        if (TryAcquire()) {
          return true;
        }
        continue;
      } else {
        if (state == States::LockedNoWaiters) {
          node->next = nullptr;
        } else {
          node->next = (WaitNode*)state;
        }
        if (state_.compare_exchange_strong(state, (State)node)) {
          return false;
        }
        continue;
      }
    }
  }

  void Release() {
    if (head_ != nullptr) {
      ResumeNextOwner(TakeNextOwner());
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
        continue ;
      }
      // Wait list
      WaitNode* waiters = (WaitNode*)state_.exchange(States::LockedNoWaiters, std::memory_order::acquire);
      head_ = ReverseList(waiters);
      ResumeNextOwner(TakeNextOwner());
      return ;
    }
  }

  FiberHandle TakeNextOwner() {
    auto next = head_;
    head_ = head_->next;
    return next->handle;
  }

  void ResumeNextOwner(FiberHandle handle) {
    auto awaiter = UnlockAwaiter(handle);
    self::Suspend(awaiter);
  }

  static WaitNode* ReverseList(WaitNode* head) {
    auto prev = head;
    auto curr = prev->next;
    while (curr != nullptr) {
      auto next = curr->next;
      curr->next = prev;
      prev = curr;
      curr = next;
    }
    head->next = nullptr;

    return prev;
  }

 private:
  std::atomic<State> state_ = States::Unlocked;
  WaitNode* head_ = nullptr;
};

using Mutex = LockFreeMutex;

//////////////////////////////////////////////////////////////////////

}  // namespace magic