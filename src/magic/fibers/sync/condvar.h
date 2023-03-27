#pragma once

#include <magic/fibers/sync/mutex.h>
#include <magic/fibers/sync/detail/futex.h>

namespace magic::fibers {

class CondVar final {
  using Lock = std::unique_lock<Mutex>;

 public:

  // ~ Public Interface

  void Wait(Lock& lock);

  void NotifyOne();
  void NotifyAll();

 private:
  detail::FutexLike<size_t> futex_;
};

}  // namespace magic::fibers