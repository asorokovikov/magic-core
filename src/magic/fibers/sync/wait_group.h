#pragma once

#include <magic/fibers/sync/oneshotevent.h>

namespace magic::fibers {

//////////////////////////////////////////////////////////////////////

class WaitGroup final {
 public:

  // ~ Public Interface

  void Add(size_t count = 1) {
    counter_.fetch_add(count);
  }

  void Done() {
    if (counter_.fetch_sub(1) == 1) {
      event_.Fire();
    }
  }

  void Wait() {
    event_.WaitAsync();
  }

 private:
  OneShotEvent event_;
  std::atomic<size_t> counter_ = 0;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic::fibers