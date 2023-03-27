#pragma once

#include <magic/common/routine.h>
#include <sure/context.hpp>
#include <wheels/memory/view.hpp>

#include <exception>

namespace magic::detail {

//////////////////////////////////////////////////////////////////////

// Stackful asymmetric coroutine impl
// - Does not manage stacks
// - Unsafe Suspend
// Base for standalone coroutines, processors, fibers

class CoroutineImpl final : public ::sure::ITrampoline {
  using Context = sure::ExecutionContext;

 public:
  CoroutineImpl(Routine routine, wheels::MutableMemView stack);

  // Context: Caller
  void Resume();

  // Context: Coroutine
  void Suspend();

  // Context: Caller
  bool IsCompleted() const;

 private:
  // sure::ITrampoline
  [[noreturn]] void Run() noexcept override;

 private:
  Routine routine_;
  Context context_;
  Context external_;
  std::exception_ptr exception_;
  bool completed_ = false;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic::detail