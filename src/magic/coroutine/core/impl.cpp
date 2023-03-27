#include <magic/coroutine/core/impl.h>

#include <wheels/core/assert.hpp>
#include <wheels/core/compiler.hpp>
#include <wheels/core/exception.hpp>

namespace magic::detail {

//////////////////////////////////////////////////////////////////////

CoroutineImpl::CoroutineImpl(Routine routine, wheels::MutableMemView stack)
    : routine_(std::move(routine)) {
  context_.Setup(stack, this);
}

void CoroutineImpl::Run() noexcept {
  try {
    routine_();
  } catch (...) {
    exception_ = std::current_exception();
  }
  completed_ = true;
  context_.ExitTo(external_);

  WHEELS_UNREACHABLE();
}

void CoroutineImpl::Resume() {
  WHEELS_ASSERT(!completed_, "Coroutine is completed");
  external_.SwitchTo(context_);
  if (exception_ != nullptr) {
    std::rethrow_exception(exception_);
  }
}

void CoroutineImpl::Suspend() {
  context_.SwitchTo(external_);
}

bool CoroutineImpl::IsCompleted() const {
  return completed_;
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic::detail