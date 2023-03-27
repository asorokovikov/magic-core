#include <magic/fibers/core/fiber.h>
#include <magic/fibers/core/stack.h>
#include <magic/concurrency/local/ptr.h>
#include <magic/common/uniqueid.h>

#include <magic/executors/thread_pool.h>
#include <magic/executors/execute.h>

#include <wheels/core/defer.hpp>

namespace magic {

//////////////////////////////////////////////////////////////////////

static ThreadLocalPtr<Fiber> this_thread_fiber;

//////////////////////////////////////////////////////////////////////

void Fiber::Create(Routine routine, IExecutor& executor) {
  static UniqueIdGenerator generator;
  auto fiber = new Fiber(generator.Next(), std::move(routine), executor);
  fiber->Schedule();
}

void Fiber::Create(Routine routine) {
  Create(std::move(routine), GetCurrentFiber().executor_);
}

Fiber& Fiber::GetCurrentFiber() {
  WHEELS_VERIFY(this_thread_fiber, "Not in a fiber context");
  return *this_thread_fiber;
}

IExecutor& Fiber::GetCurrentExecutor() {
  return GetCurrentFiber().executor_;
}

//////////////////////////////////////////////////////////////////////

Fiber::Fiber(FiberId id, Routine routine, IExecutor& executor)
    : stack_(AllocateStack()),
      coroutine_(std::move(routine), stack_.MutView()),
      executor_(executor),
      state_(FiberState::Pending),
      id_(id),
      awaiter_(nullptr) {
}

void Fiber::Schedule() {
  state_ = FiberState::Queued;
  executor_.Execute(this);
}

auto Fiber::GetFiberId() const -> FiberId {
  return id_;
}

void Fiber::Suspend() {
  state_ = FiberState::Suspended;
  coroutine_.Suspend();
}

void Fiber::Suspend(ISuspendAwaiter* awaiter) {
  awaiter_ = awaiter;
  Suspend();
}

void Fiber::Resume() {
  WHEELS_VERIFY(state_ == FiberState::Suspended, "Expected fiber in Suspended state");
  Schedule();
}

// Interface: ITask
void Fiber::Run() noexcept {
  auto next = this;
  while (next) {
    next = next->RunFiber();
  }
}

void Fiber::Discard() noexcept {
  Destroy();
}

Fiber* Fiber::RunFiber() {
  Step();

  if (coroutine_.IsCompleted()) {
    Destroy();
    return nullptr;
  }

  WHEELS_ASSERT(state_ == FiberState::Suspended, "Unexpected fiber state");

  auto awaiter = std::exchange(awaiter_, nullptr);
  if (awaiter) {
    auto next = awaiter->OnCompleted(this);
    if (next.IsValid()) {
      return next.fiber_;
    }
  }

  return nullptr;
}

void Fiber::Step() {
  state_ = FiberState::Running;

  auto prev = this_thread_fiber.Exchange(this);
  auto rollback = wheels::Defer([prev] {
    this_thread_fiber = prev;
  });

  coroutine_.Resume();
}

void Fiber::Destroy() {
  ReleaseStack(std::move(stack_));
  delete this;
}

//////////////////////////////////////////////////////////////////////

// API Implementation

// Starts a new fiber in a new scheduler
void RunScheduler(size_t threads, Routine routine) {
  auto scheduler = ThreadPool(threads);

  Go(scheduler, std::move(routine));

  scheduler.WaitIdle();
  scheduler.Stop();
}

// Starts a new fiber
void Go(Scheduler& executor, Routine routine) {
  Fiber::Create(std::move(routine), executor);
}

// Starts a new fiber in the current scheduler
void Go(Routine routine) {
  Fiber::Create(std::move(routine));
}

//////////////////////////////////////////////////////////////////////

namespace self {

void Yield() {
  auto awaiter = YieldAwaiter();
  Fiber::GetCurrentFiber().Suspend(&awaiter);
}

void Suspend(ISuspendAwaiter& awaiter) {
  Fiber::GetCurrentFiber().Suspend(&awaiter);
}

auto GetFiberId() -> FiberId {
  return Fiber::GetCurrentFiber().GetFiberId();
}

}  // namespace self

//////////////////////////////////////////////////////////////////////

}  // namespace magic