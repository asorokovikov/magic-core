#pragma once

#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <cassert>

namespace magic {

// CyclicBarrier allows a set of threads to all wait for each other
// to reach a common barrier point

// The barrier is called cyclic because
// it can be re-used after the waiting threads are released.

class CyclicBarrier final {
 public:
  explicit CyclicBarrier(size_t participants) : participants_(participants) {
    assert(participants > 0);
  }

  // Blocks until all participants have invoked Arrive()
  void ArriveAndWait() {
    std::unique_lock lock(mutex_);

    auto this_thread_generation = generation_;
    if (++arrived_count == participants_) {
      ++generation_;
      arrived_count = 0;
      all_arrived_.notify_all();
    }

    while (this_thread_generation == generation_) {
      all_arrived_.wait(lock);
    }
  }

 private:
  const size_t participants_;
  size_t generation_ = 0;
  size_t arrived_count = 0;

  std::mutex mutex_;
  std::condition_variable all_arrived_;
};

}  // namespace magic