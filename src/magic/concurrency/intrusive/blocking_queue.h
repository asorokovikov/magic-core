#pragma once

#include <wheels/intrusive/forward_list.hpp>

#include <mutex>
#include <condition_variable>
#include <optional>

namespace magic {

//////////////////////////////////////////////////////////////////////

// // Multi-producer/multi-consumer unbounded blocking intrusive queue

template <typename T>
class MPMCBlockingQueue final {
  using ForwardList = wheels::IntrusiveForwardList<T>;

 public:
  // Returns false if queue is closed

  bool Put(T* item) {
    std::lock_guard lock(mutex_);
    if (closed_) {
      return false;
    }
    items_.PushBack(item);
    not_empty_.notify_one();

    return true;
  }

  // Await and take a next item

  T* Take() {
    std::unique_lock lock(mutex_);
    while (items_.IsEmpty()) {
      if (closed_) {
        return nullptr;
      }
      not_empty_.wait(lock);
    }

    return items_.PopFront();
  }

  // Close queue for producers

  void Close() {
    CloseImpl(false, [](T*) {});
  }

  // Close queue for producers and consumers and discard existing items

  template <typename Func>
  void Shutdown(Func&& disposer) {
    CloseImpl(true, std::forward<Func>(disposer));
  }

  private:
  template <typename Func>
  void CloseImpl(bool clear, Func&& disposer) {
    std::lock_guard guard(mutex_);
    if (clear) {
      while (items_.HasItems()) {
        disposer(items_.PopFront());
      }
    }
    closed_ = true;
    not_empty_.notify_all();
  }

 private:
  ForwardList items_;
  bool closed_ = false;

  std::mutex mutex_;
  std::condition_variable not_empty_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic