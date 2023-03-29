#pragma once

#include <wheels/core/assert.hpp>

#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace magic {

//////////////////////////////////////////////////////////////////////

// Unbounded Blocking Multi-Producer / Multi-Consumer Queue (MPMC)
// class UnboundedBlockingMPMCQueue

template <typename T>
class UnboundedBlockingQueue final {
 public:
  // Thread role: producer
  bool Put(T value) {
    std::lock_guard guard(mutex_);

    if (closed_) {
      return false;
    }
    buffer_.emplace_back(std::move(value));
    not_empty_.notify_one();

    return true;
  }

  // Thread role: consumer
  std::optional<T> Take() {
    std::unique_lock lock(mutex_);

    while (buffer_.empty()) {
      if (closed_) {
        return std::nullopt;
      }
      not_empty_.wait(lock);
    }

    return TakeLocked();
  }

  void Close() {
    std::lock_guard guard(mutex_);
    closed_ = true;
    buffer_.clear();
    not_empty_.notify_all();
  }

  template <typename Func>
  void Shutdown(Func&& disposer) {
    std::lock_guard guard(mutex_);
    for (auto&& item : buffer_) {
      disposer(std::move(item));
    }
    closed_ = true;
    buffer_.clear();
    not_empty_.notify_all();
  }

 private:
  auto TakeLocked() -> T {
    WHEELS_ASSERT(!buffer_.empty(), "Buffer is empty");
    T value = std::move(buffer_.front());
    buffer_.pop_front();
    return value;
  }

 private:
  std::deque<T> buffer_;
  std::mutex mutex_;
  std::condition_variable not_empty_;

  bool closed_ = false;
};

//////////////////////////////////////////////////////////////////////

}  // namespace name