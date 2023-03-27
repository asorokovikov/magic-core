#pragma once

#include <magic/coroutine/stackless/dispatch.h>

namespace magic {

// Precondition: coroutine is running in `current` executor
inline auto Yield(IExecutor& current) {
  return DispatchTo(current);
}

}  // namespace magic