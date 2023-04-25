#pragma once

#include <magic/common/refer/ref_counted.h>
#include <magic/executors/executor.h>
#include <magic/concurrency/lockfree/lockfree_intrusive_queue.h>

namespace magic {

namespace detail {

class StrandImpl : public TaskNode, public RefCounted<StrandImpl> {
  using TaskQueue = MPSCLockFreeIntrusiveQueue<TaskNode>;

 public:
  explicit StrandImpl(IExecutor& e) : executor_(e) {
  }

  void Execute(TaskNode* task);

  // TaskNode
  void Run() noexcept override;
  void Discard() noexcept override;

 private:
  void RunNextBatch();

 private:
  IExecutor& executor_;
  TaskQueue tasks_;
  std::atomic<size_t> counter_ = 0;
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

// Strand (serial executor, asynchronous mutex)

class Strand final : public IExecutor {
 public:
  Strand(IExecutor& executor);

  // IExecutor
  void Execute(TaskNode* task) override;

  IExecutor& AsExecutor() {
    return *this;
  }

 private:
  Ref<detail::StrandImpl> impl_;
};

//////////////////////////////////////////////////////////////////////

}  // namespace magic