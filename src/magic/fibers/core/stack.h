#pragma once

#include <sure/stack.hpp>
#include <magic/fibers/core/metrics.h>

namespace magic {

sure::Stack AllocateStack();

void ReleaseStack(sure::Stack stack);

AllocatorMetrics GetAllocatorMetrics();

} // namespace magic