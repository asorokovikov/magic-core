#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace magic {

class AtomicCounter final {
 public:

  // ~ Public Interface

  void Add(size_t count = 1){
    counter_.fetch_add(count, std::memory_order::relaxed);
  }

  void Done() {
    if (counter_.fetch_sub(1, std::memory_order::acq_rel) == 1) {
      std::lock_guard lock(mutex_);
      all_done_.notify_all();
    }
  }

  void WaitZero() {
    std::unique_lock lock(mutex_);
    while (counter_.load() > 0) {
      all_done_.wait(lock);
    }
  }

 private:
  std::atomic<size_t> counter_ = 0;
  std::mutex mutex_;
  std::condition_variable all_done_;
};

}  // namespace magic