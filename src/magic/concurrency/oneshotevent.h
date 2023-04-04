#pragma once

#include <mutex>

namespace magic {

class OneShotEvent {
 public:
  void Wait() {
    std::unique_lock locker(mutex_);
    while (!fired_) {
      fired_cond_.wait(locker);
    }
  }

  void Fire() {
    std::lock_guard guard(mutex_);
    fired_ = true;
    fired_cond_.notify_one();
  }

 private:
  bool fired_ = false;
  std::mutex mutex_;
  std::condition_variable fired_cond_;
};

}  // namespace magic