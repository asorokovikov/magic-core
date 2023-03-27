#include <magic/fibers/core/handle.h>
#include <magic/fibers/core/fiber.h>

#include <wheels/core/assert.hpp>

#include <utility>

namespace magic {

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

FiberHandle FiberHandle::Invalid() {
  return FiberHandle(nullptr);
}

void FiberHandle::Schedule() {
  Release()->Schedule();
}

void FiberHandle::Resume() {
  Release()->Resume();
}

Fiber* FiberHandle::Release() {
  WHEELS_ASSERT(fiber_ != nullptr, "Invalid fiber handler");
  return std::exchange(fiber_, nullptr);
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic