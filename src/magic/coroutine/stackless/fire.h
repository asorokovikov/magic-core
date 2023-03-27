#pragma once

#include <magic/coroutine/stackless/task.h>

namespace magic {

inline void FireAndForget(Task<>&& task) {
  task.ReleaseCoroutine().resume();
}

}  // namespace magic