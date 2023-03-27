#pragma once

#include <magic/common/intrusive/forward_list.h>

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

// LockFree mutex
class Mutex final {
  using State = uint64_t;

  struct States {
    enum _ {
      Unlocked = 0,
      LockedNoWaiters = 1,
      // + WaitNode address
    };
  };

  struct Locker : public IMaybeSuspendAwaiter, public IntrusiveForwardListNode<Locker> {
    Locker(Mutex& mutex) : mutex_(mutex) {
    }

    // Interface: IMaybeSuspendAwaiter
    bool AwaitSuspend(FiberHandle handle) override {
      assert(handle.IsValid());
      handle_ = handle;
      return mutex_.TryLockOrEnqueue(this);
    }

    FiberHandle GetFiberHandle() const {
      return handle_;
    }

   private:
    Mutex& mutex_;
    FiberHandle handle_;
  };

  struct Unlocker : ISuspendAwaiter {
    Unlocker(FiberHandle next) : next_(next) {
    }

    FiberHandle OnCompleted(FiberHandle handle) override {
      auto next = next_;  // This awaiter can be destroyed after next line!
      handle.Schedule();
      return next;
    }

   private:
    FiberHandle next_;
  };

  //////////////////////////////////////////////////////////////////////

 public:

  // ~ Public Interface

  bool TryLock() {
    State state = States::Unlocked;
    return state_.compare_exchange_strong(state, States::LockedNoWaiters,
                                          std::memory_order::acquire);
  }

  void Lock() {
    if (TryLock()) {
      return;  // Fast path
    }
    auto awaiter = Locker(*this);
    self::Suspend(awaiter);
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
  using Node = IntrusiveForwardListNode<Locker>;
  using WaiterList = IntrusiveForwardList<Locker>;
  using AtomicState = std::atomic<State>;

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
    auto unlocker = Unlocker(next->GetFiberHandle());
    self::Suspend(unlocker);
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
  AtomicState state_ = States::Unlocked;
  WaiterList waiters_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic::fibers