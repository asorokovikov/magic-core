#pragma once

#include <magic/coroutine/stackless/sync/oneshotevent.h>

namespace magic {

class WaitGroup {
 public:
  void Add(size_t count = 1) {
    counter_.fetch_add(count);
  }

  void Done() {
    if (counter_.fetch_sub(1) == 1) {
      event_.Fire();
    }
  }

  auto WaitAsync() {
    return event_.WaitAsync();
  }

 private:
  std::atomic<size_t> counter_ = 0;
  OneShotEvent event_;
};

}  // namespace magic