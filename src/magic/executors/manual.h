#pragma once

#include <magic/executors/executor.h>
#include <magic/concurrency/lockfree/lockfree_intrusive_queue.h>

#include <deque>

namespace magic {

//////////////////////////////////////////////////////////////////////

// Single-threaded task queue for deterministic testing

class ManualExecutor final : public IExecutor {
 public:

  // IExecutor
  void Execute(TaskNode* task) override;

  // Run tasks until queue is empty
  // Returns the number of completed tasks
  // Post-condition: HasTasks() == false
  size_t RunAll();

  // Run at most 'limit' tasks from queue
  // Returns the number of completed tasks
  size_t RunAtMost(size_t limit);

  // Run task if queue is not empty
  bool RunOnce() {
    return RunAtMost(1) == 1;
  }

  // Returns the number of tasks that are queued
  size_t PendingTasks() const {
    return tasks_.Size();
  }

  bool HasTasks() const {
    return tasks_.HasItems();
  }

  ~ManualExecutor();

 private:
  void RunNextTask();

 private:
  wheels::IntrusiveForwardList<TaskNode> tasks_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic