#pragma once

#include <magic/fibers/sync/mutex.h>
#include <magic/concurrency/lockfree/lockfree_intrusive_stack.h>

namespace magic::fibers {

//////////////////////////////////////////////////////////////////////

namespace detail {

struct WaitNode {
  FiberHandle fiber;
  WaitNode* next = nullptr;
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

class WaitGroup final {
  using UniqueLock = std::unique_lock<Mutex>;

  struct LockAwaiter : public IAlwaysSuspendAwaiter,
                       public wheels::IntrusiveForwardListNode<LockAwaiter> {
    LockAwaiter(UniqueLock lock) : lock_(std::move(lock)) {
    }

    void AwaitSuspend(FiberHandle handle) override {
      assert(handle.IsValid());
      fiber_ = handle;
      lock_.unlock();
    }

    void Resume() {
      fiber_.Resume();
    }

   private:
    UniqueLock lock_;
    FiberHandle fiber_;
  };

  using Stack = MPSCLockFreeIntrusiveStack<LockAwaiter>;

 public:

  // ~ Public Interface

  void Add(size_t count = 1) {
    counter_.fetch_add(count);
  }

  void Done() {
    if (counter_.fetch_sub(1) == 1) {

    }

    if (--counter_ == 0) {
      waiters_.ConsumeAll([](auto waiter) {
        waiter->Resume();
      });
    }
  }

  void Wait() {
    if (counter_ > 0) {
      auto awaiter = LockAwaiter(std::move(lock));
      waiters_.Push(&awaiter);
      self::Suspend(awaiter);
    }
  }

  //////////////////////////////////////////////////////////////////////

 private:
  Stack waiters_;

  std::atomic<size_t> counter_ = 0;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic::fibers