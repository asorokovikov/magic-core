#pragma once

#include <magic/fibers/core/awaiter.h>
#include <magic/executors/execute.h>
#include <magic/common/routine.h>

namespace magic {

using Scheduler = IExecutor;

using FiberId = size_t;

// Public Interface

// Starts a new fiber in a new scheduler
void RunScheduler(size_t threads, Routine routine);

// Starts a new fiber and specify a scheduler
void Go(IExecutor& executor, Routine routine);

// Starts a new fiber in the current scheduler
void Go(Routine routine);

//////////////////////////////////////////////////////////////////////

namespace self {

void Yield();

void Suspend(ISuspendAwaiter& awaiter);

auto GetFiberId() -> FiberId;

bool IsFiber();

} // namespace self

//////////////////////////////////////////////////////////////////////

} // namespace magic