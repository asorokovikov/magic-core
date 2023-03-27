#pragma once

#include <magic/fibers/core/handle.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

/// Represents an operation that will schedule continuations when the fiber suspend completes
struct ISuspendAwaiter {
  virtual FiberHandle OnCompleted(FiberHandle handle) = 0;
};

//////////////////////////////////////////////////////////////////////

struct IMaybeSuspendAwaiter : ISuspendAwaiter {
  virtual FiberHandle OnCompleted(FiberHandle handle) override {
    if (AwaitSuspend(handle)) {
      return handle;
    }
    return FiberHandle::Invalid();
  }

  virtual bool AwaitSuspend(FiberHandle handle) = 0;
};

//////////////////////////////////////////////////////////////////////

struct IAlwaysSuspendAwaiter : ISuspendAwaiter {
  virtual FiberHandle OnCompleted(FiberHandle handle) override {
    AwaitSuspend(handle);
    return FiberHandle::Invalid(); // always suspend
  }

  virtual void AwaitSuspend(FiberHandle handle) = 0;
};

//////////////////////////////////////////////////////////////////////

struct YieldAwaiter : ISuspendAwaiter {
  FiberHandle OnCompleted(FiberHandle handler) override {
    handler.Schedule();
    return FiberHandle::Invalid();
  }
};

}  // namespace magic