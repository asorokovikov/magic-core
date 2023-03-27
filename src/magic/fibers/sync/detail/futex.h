#pragma once

#include <magic/fibers/api.h>
#include <magic/fibers/core/awaiter.h>
#include <magic/concurrency/spinlock.h>

#include <wheels/intrusive/list.hpp>
#include <wheels/core/assert.hpp>

namespace magic::fibers::detail {

//////////////////////////////////////////////////////////////////////

template <typename T>
class FutexLike final {
  using UniqueLock = std::unique_lock<SpinLock>;

  class FutexAwaiter : public IAlwaysSuspendAwaiter,
                       public wheels::IntrusiveListNode<FutexAwaiter> {
   public:
    FutexAwaiter(UniqueLock&& lock) : lock_(std::move(lock)) {
    }

    void AwaitSuspend(FiberHandle handle) override {
      WHEELS_ASSERT(handle.IsValid(), "Invalid fiber handler");
      handle_ = handle;
      lock_.unlock();
    }

    void Resume() {
      handle_.Resume();
    }

   private:
    UniqueLock lock_;
    FiberHandle handle_;
  };

  //////////////////////////////////////////////////////////////////////

  using WaitList = wheels::IntrusiveList<FutexAwaiter>;
  using WaitKey = size_t;

 public:
  ~FutexLike() {
    WHEELS_VERIFY(waiters_.IsEmpty(), "Waiters list is not empty");
  }

  // ~ Public Interface

  WaitKey PrepareWait() const {
    return epoch.load();
  }

  void ParkIfEqual(WaitKey old) {
    auto lock = UniqueLock(spinlock_);
    if (epoch.load() == old) {
      auto awaiter = FutexAwaiter(std::move(lock));
      waiters_.PushBack(&awaiter);
      self::Suspend(awaiter);
    }
  }

  bool WakeOne() {
    auto list = WaitList();
    {
      auto lock = UniqueLock(spinlock_);
      epoch.fetch_add(1);
      if (waiters_.HasItems()) {
        list.PushBack(waiters_.PopFront());
      }
    }
    return Wake(std::move(list)) > 0;
  }

  size_t WakeAll() {
    auto list = WaitList();
    {
      auto lock = UniqueLock(spinlock_);
      epoch.fetch_add(1);
      list.Append(waiters_);
    }
    return Wake(std::move(list));
  }

  //////////////////////////////////////////////////////////////////////

 private:
  size_t Wake(WaitList list) {
    auto count = 0ul;
    while (list.HasItems()) {
      auto item = list.PopFront();
      item->Resume();
      ++count;
    }
    return count;
  }

 private:
  SpinLock spinlock_;
  WaitList waiters_;
  std::atomic<size_t> epoch = 0;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic::fibers::detail