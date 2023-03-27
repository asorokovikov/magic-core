#pragma once

#include <magic/coroutine/core/impl.h>
#include <sure/stack.hpp>

namespace magic {

class Coroutine final {
  using Stack = sure::Stack;

 public:
  explicit Coroutine(Routine routine);

  // Suspend current coroutine
  static void Suspend();

  void Resume();

  bool IsCompleted() const;

 private:
  Stack stack_;
  detail::CoroutineImpl impl_;
};

}  // namespace magic