#pragma once

#include <magic/fibers/api.h>
#include <magic/coroutine/core/impl.h>

#include <magic/executors/executor.h>
#include <magic/executors/strand.h>

#include <sure/stack.hpp>

namespace magic {

//////////////////////////////////////////////////////////////////////

enum class FiberState {
    Pending,  // The fiber is waiting for a worker to pick it up
    Queued,   // The fiber is already added in run queue
    Running,
    Suspended,  // The fiber is not active because it is waiting on some resource
};

//////////////////////////////////////////////////////////////////////

// Fiber = Stackful coroutine + Scheduler

class Fiber final : public TaskNode {
    using Stack = sure::Stack;
    using Coroutine = detail::CoroutineImpl;

public:
    static void Create(Routine routine);
    static void Create(Routine routine, IExecutor& scheduler);

    static Fiber& GetCurrentFiber();
    static IExecutor& GetCurrentExecutor();

    // ~ System calls

    FiberId GetFiberId() const;

    void Resume();
    void Suspend();
    void Suspend(ISuspendAwaiter* awaiter);
    void Schedule();

    // Interface: ITask
    void Run() noexcept override;
    void Discard() noexcept override;

private:
    Fiber(FiberId id, Routine routine, IExecutor& scheduler);

    void Step();
    void Destroy();

    Fiber* RunFiber();

private:
    Stack stack_;
    Coroutine coroutine_;
    IExecutor& executor_;

    FiberState state_;
    FiberId id_;

    ISuspendAwaiter* awaiter_;
};

//////////////////////////////////////////////////////////////////////

} // namespace magic