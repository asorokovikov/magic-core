#include <magic/executors/manual.h>
#include <wheels/core/assert.hpp>

namespace magic {

//////////////////////////////////////////////////////////////////////

ManualExecutor::~ManualExecutor() {
  WHEELS_ASSERT(tasks_.IsEmpty(), "Destroying Manual executor with non-empty task queue");
}

void ManualExecutor::Execute(TaskNode* task) {
  tasks_.PushBack(task);
}

size_t ManualExecutor::RunAll() {
  auto completed = 0ul;
  while (tasks_.HasItems()) {
    RunNextTask();
    ++completed;
  }

  return completed;
}

size_t ManualExecutor::RunAtMost(size_t limit) {
  auto completed = 0ul;
  while (completed < limit && HasTasks()) {
    RunNextTask();
    ++completed;
  }

  return completed;
}

void ManualExecutor::RunNextTask() {
  auto task = tasks_.PopFront();
  task->Run();
}

//////////////////////////////////////////////////////////////////////

}  // namespace magic