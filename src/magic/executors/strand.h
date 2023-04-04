#pragma once

#include <magic/executors/executor.h>
#include <magic/concurrency/lockfree/lockfree_intrusive_queue.h>

namespace magic {

//////////////////////////////////////////////////////////////////////

// Strand (serial executor, asynchronous mutex)

class Strand final : public IExecutor, public TaskNode {
  using Queue = MPSCLockFreeIntrusiveQueue<TaskNode>;
  using Atomic = std::atomic<size_t>;

 public:
  Strand(IExecutor& executor);

  // IExecutor
  void Execute(TaskNode* task) override;

  void Run() noexcept override;
  void Discard() noexcept override;

  IExecutor& GetExecutor() const {
    return executor_;
  }

 private:
  void RunNextBatch();

 private:
  IExecutor& executor_;
  Queue tasks_;
  Atomic counter_ = 0;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic