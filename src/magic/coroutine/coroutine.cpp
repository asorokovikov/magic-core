#include "coroutine.h"

#include <magic/fibers/core/stack.h>
#include <magic/concurrency/local/ptr.h>

#include <wheels/core/assert.hpp>
#include <wheels/core/defer.hpp>

namespace magic {

//////////////////////////////////////////////////////////////////////

static ThreadLocalPtr<Coroutine> current;

//////////////////////////////////////////////////////////////////////

Coroutine::Coroutine(Routine routine)
    : stack_(AllocateStack()), impl_(std::move(routine), stack_.MutView()) {
}

void Coroutine::Resume() {
  auto prev = current.Exchange(this);

  wheels::Defer rollback([prev]() {
    current = prev;
  });

  impl_.Resume();
}

void Coroutine::Suspend() {
  WHEELS_VERIFY(current, "Not a coroutine");
  current->impl_.Suspend();
}

bool Coroutine::IsCompleted() const {
  return impl_.IsCompleted();
}

}  // namespace magic