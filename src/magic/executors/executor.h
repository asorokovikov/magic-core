#pragma once

#include <magic/executors/task.h>

namespace magic {

struct IExecutor {
  virtual ~IExecutor() = default;

  virtual void Execute(TaskNode* task) = 0;
};

}  // namespace magic